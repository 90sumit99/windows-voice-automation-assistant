"""
zyrex_brain.py — ZYREX Voice Brain v2
Complete voice pipeline:
  Mic capture → silence detection → Whisper STT
  → Intent parse → C++ bridge
"""

import threading
import time
import re
import os
import numpy as np

from zyrex_state import state, ZyrexStatus, ModuleStatus, LogLevel
from zyrex_bridge import bridge
from zyrex_ai import ai

# ─────────────────────────────────────────────────────────────
#  CONFIG  — tune these for your mic
# ─────────────────────────────────────────────────────────────
WHISPER_MODEL      = "base"    # tiny=fastest, base=best balance, small=most accurate
SAMPLE_RATE        = 16000     # whisper needs 16kHz
CHUNK_SIZE         = 1024      # audio chunk size
SILENCE_THRESHOLD  = 0.004     # RMS below this = silence (lower = more sensitive)
SILENCE_DURATION   = 1.2       # seconds of silence before stopping recording
MAX_RECORD_SECONDS = 8         # max recording length
MIN_SPEECH_SECONDS = 0.4       # ignore clips shorter than this
WAVEFORM_BARS      = 32        # number of waveform bars in UI
WAKE_WORD          = "hey zyrex"
USE_WAKE_WORD      = os.getenv("ZYREX_USE_WAKE_WORD", "0").strip().lower() in {"1", "true", "yes"}
NOISE_CALIBRATE_SECONDS = 0.8


# ─────────────────────────────────────────────────────────────
#  SYSTEM STATS UPDATER
# ─────────────────────────────────────────────────────────────
class SystemStatsUpdater(threading.Thread):
    def __init__(self):
        super().__init__(daemon=True)

    def run(self):
        while True:
            try:
                import psutil
                cpu  = psutil.cpu_percent(interval=1)
                ram  = psutil.virtual_memory().percent
                disk = psutil.disk_usage('/').percent

                wifi = False
                try:
                    for iface, s in psutil.net_if_stats().items():
                        if s.isup and any(x in iface.lower()
                                          for x in ['wi','wlan','wireless']):
                            wifi = True; break
                except Exception:
                    pass

                battery, plugged = 100, True
                try:
                    bat = psutil.sensors_battery()
                    if bat:
                        battery = int(bat.percent)
                        plugged = bat.power_plugged
                except Exception:
                    pass

                state.set_system_stats(cpu, ram, disk, wifi, battery, plugged)
            except Exception:
                pass
            time.sleep(2)


# ─────────────────────────────────────────────────────────────
#  VOICE ENGINE  — faster-whisper STT
# ─────────────────────────────────────────────────────────────
class VoiceEngine:
    def __init__(self):
        self.model     = None
        self.is_loaded = False

    def load(self) -> bool:
        try:
            state.info(f"Loading Whisper ({WHISPER_MODEL})...")
            from faster_whisper import WhisperModel
            self.model = WhisperModel(
                WHISPER_MODEL,
                device="cpu",
                compute_type="int8"
            )
            self.is_loaded = True
            state.set_module("Voice Engine", ModuleStatus.ONLINE)
            state.success(f"Whisper ({WHISPER_MODEL}) ready.")
            return True
        except ImportError:
            state.error("faster-whisper not installed. Run: pip install faster-whisper")
            return False
        except Exception as e:
            state.error(f"Whisper load failed: {e}")
            return False

    def transcribe(self, audio: np.ndarray) -> str:
        if not self.is_loaded or self.model is None:
            return ""
        try:
            audio = audio.astype(np.float32)
            # normalize audio
            if audio.max() > 0:
                audio = audio / audio.max() * 0.95

            segs, info = self.model.transcribe(
                audio,
                language="en",
                beam_size=5,
                vad_filter=True,
                vad_parameters=dict(
                    min_silence_duration_ms=500,
                    speech_pad_ms=200
                )
            )
            text = " ".join(s.text.strip() for s in segs).strip()

            # filter out whisper hallucinations
            bad = ["thank you", "thanks for watching", "subscribe",
                   "you", ".", ",", "the", ""]
            if text.lower() in bad or len(text) < 2:
                return ""

            return text
        except Exception as e:
            state.error(f"Transcription error: {e}")
            return ""


# ─────────────────────────────────────────────────────────────
#  SMART AUDIO CAPTURE
#  Records until silence detected — not a fixed duration
# ─────────────────────────────────────────────────────────────
class SmartAudioCapture:
    def __init__(self):
        self._active = False
        self._threshold = SILENCE_THRESHOLD

    def _calibrate_threshold(self, stream) -> float:
        """
        Measure ambient mic RMS and set a dynamic threshold.
        This avoids hard-coded values that fail on many microphones.
        """
        chunks = max(1, int((NOISE_CALIBRATE_SECONDS * SAMPLE_RATE) / CHUNK_SIZE))
        ambient: list[float] = []
        for _ in range(chunks):
            chunk, _ = stream.read(CHUNK_SIZE)
            flat = chunk.flatten()
            rms = float(np.sqrt(np.mean(flat ** 2)))
            ambient.append(rms)
        base = float(np.percentile(ambient, 75)) if ambient else SILENCE_THRESHOLD
        # Keep within sane bounds; multiplier keeps sensitivity above room noise.
        return max(0.0015, min(0.03, base * 2.4))

    def record_until_silence(self) -> np.ndarray:
        """
        Records audio from mic.
        Stops automatically when silence is detected.
        Updates waveform bars in real time.
        """
        try:
            import sounddevice as sd

            state.set_status(ZyrexStatus.LISTENING)
            state.info("Listening...")

            frames       = []
            silent_time  = 0.0
            speech_found = False
            chunk_dur    = CHUNK_SIZE / SAMPLE_RATE  # seconds per chunk

            self._active = True

            with sd.InputStream(
                samplerate=SAMPLE_RATE,
                channels=1,
                dtype='float32',
                blocksize=CHUNK_SIZE
            ) as stream:
                self._threshold = self._calibrate_threshold(stream)
                state.info(f"Mic threshold auto-set: {self._threshold:.4f}")
                elapsed = 0.0

                while elapsed < MAX_RECORD_SECONDS:
                    chunk, _ = stream.read(CHUNK_SIZE)
                    flat      = chunk.flatten()
                    frames.append(flat.copy())
                    elapsed  += chunk_dur

                    # RMS energy for silence detection
                    rms = float(np.sqrt(np.mean(flat ** 2)))

                    # Update waveform in UI
                    wv = []
                    seg = max(1, len(flat) // WAVEFORM_BARS)
                    for i in range(WAVEFORM_BARS):
                        chunk_rms = float(np.sqrt(np.mean(
                            flat[i*seg:(i+1)*seg] ** 2)))
                        wv.append(min(1.0, chunk_rms * 12))
                    state.set_waveform(wv)

                    # Detect speech start
                    if rms > self._threshold:
                        speech_found = True
                        silent_time  = 0.0
                    elif speech_found:
                        silent_time += chunk_dur
                        if silent_time >= SILENCE_DURATION:
                            break  # stop on silence after speech

            self._active = False
            state.set_waveform([0.0] * WAVEFORM_BARS)

            if not speech_found:
                return np.array([])

            audio = np.concatenate(frames)

            # check minimum speech duration
            if len(audio) / SAMPLE_RATE < MIN_SPEECH_SECONDS:
                return np.array([])

            return audio

        except ImportError:
            state.error("sounddevice not installed. Run: pip install sounddevice")
            return np.array([])
        except Exception as e:
            state.error(f"Mic capture error: {e}")
            return np.array([])

    def stop(self):
        self._active = False


# ─────────────────────────────────────────────────────────────
#  MIC TEST  — tests mic before starting
# ─────────────────────────────────────────────────────────────
def test_microphone() -> bool:
    try:
        import sounddevice as sd
        devices = sd.query_devices()
        input_devices = [d for d in devices if d['max_input_channels'] > 0]

        if not input_devices:
            state.error("No microphone found! Check your mic connection.")
            return False

        default = sd.query_devices(kind='input')
        state.success(f"Mic found: {default['name']}")
        return True

    except ImportError:
        state.error("sounddevice not installed.")
        return False
    except Exception as e:
        state.error(f"Mic test failed: {e}")
        return False


# ─────────────────────────────────────────────────────────────
#  INTENT PARSER
# ─────────────────────────────────────────────────────────────
class IntentParser:
    def parse(self, text: str) -> str:
        t = text.lower().strip()
        if not t:
            return ""

        # Exit
        if any(w in t for w in ["exit zyrex","quit zyrex","bye zyrex"]):
            return "exit"

        # Power
        if any(w in t for w in ["shut down","shutdown","turn off the computer"]):
            return "shutdown"
        if any(w in t for w in ["restart","reboot"]):
            return "restart"
        if "sleep" in t and "spotify" not in t:
            return "sleep"
        if any(w in t for w in ["lock","lock screen","lock the screen"]):
            return "lock"

        # Volume
        if "unmute" in t:
            return "unmute"
        if "mute" in t:
            return "mute"
        if any(w in t for w in ["volume up","louder","turn it up","increase volume"]):
            return "volume up"
        if any(w in t for w in ["volume down","quieter","turn it down","decrease volume","lower volume"]):
            return "volume down"
        if "volume" in t or "set the volume" in t:
            nums = re.findall(r'\d+', t)
            if nums:
                return f"volume {nums[0]}"

        # Media
        if any(w in t for w in ["next song","next track","skip","skip song"]):
            return "next track"
        if any(w in t for w in ["previous song","prev song","go back","last song"]):
            return "previous track"
        if any(w in t for w in ["play pause","pause the music","resume music","pause music"]):
            return "play"

        # Music with target
        if any(w in t for w in ["play","listen to","put on"]):
            for platform in ["spotify","youtube music","youtube","chrome"]:
                if platform in t:
                    song = t
                    for noise in ["play","listen","to","put","on","in","via",
                                  platform,"music","please"]:
                        song = song.replace(noise, "").strip()
                    song = " ".join(song.split())
                    if song:
                        return f"play {song} on {platform}"
                    return f"play on {platform}"

        # Screenshot
        if any(w in t for w in ["screenshot","take a screenshot",
                                  "capture the screen","screen capture"]):
            return "screenshot"

        # File operations
        if any(w in t for w in ["find","search for","locate","where is"]):
            q = t
            for n in ["find","search","for","locate","where","is","the","my","a"]:
                q = (" " + q + " ").replace(f" {n} ", " ").strip()
            return f"find {q}" if q else ""

        if "delete" in t:
            tgt = t.replace("delete","").replace("file","").strip()
            return f"delete {tgt}"

        # Close app
        if any(w in t for w in ["close","kill","terminate","exit"]):
            tgt = t
            for n in ["close","kill","terminate","exit","the","app","application"]:
                tgt = (" " + tgt + " ").replace(f" {n} ", " ").strip()
            return f"close {tgt}" if tgt else ""

        # Open app
        if any(w in t for w in ["open","launch","start"]):
            tgt = t
            for n in ["open","launch","start","the","please","app","application"]:
                tgt = (" " + tgt + " ").replace(f" {n} ", " ").strip()
            return f"open {tgt}" if tgt else ""

        # System info
        if any(w in t for w in ["what's my ip","ip address","my ip"]):
            return "ip"
        if any(w in t for w in ["wifi status","check wifi","network status"]):
            return "wifi"
        if any(w in t for w in ["battery","battery status","battery level"]):
            return "battery"

        # Fallback — send raw to C++ IntentResolver
        return t


# ─────────────────────────────────────────────────────────────
#  ZYREX BRAIN
# ─────────────────────────────────────────────────────────────
class ZyrexBrain:
    def __init__(self):
        self.voice   = VoiceEngine()
        self.capture = SmartAudioCapture()
        self._running = False
        self._thread  = None

    def start(self):
        state.info("Starting Zyrex brain...")
        state.set_module("AI Brain", ModuleStatus.ONLINE)

        # System stats
        SystemStatsUpdater().start()

        # Test mic
        mic_ok = test_microphone()
        if not mic_ok:
            state.warning("Microphone not detected — voice commands disabled.")
            state.warning("Text commands still work via the input box.")

        # Load Whisper
        whisper_ok = self.voice.load()
        if not whisper_ok:
            state.warning("Voice engine offline — text commands still work.")

        # Connect C++ bridge
        bridge_ok = bridge.start()
        if not bridge_ok:
            state.warning("C++ bridge offline — build VoiceAutomationCore.exe.")

        # Mark ready
        state.set_status(ZyrexStatus.IDLE)
        state.success("ZYREX ready. Speak a command or type below.")

        # Start voice loop only if both mic + whisper work
        if mic_ok and whisper_ok:
            self._running = True
            self._thread  = threading.Thread(
                target=self._voice_loop, daemon=True)
            self._thread.start()
            if USE_WAKE_WORD:
                state.success("Voice listening active (wake word mode: 'Hey Zyrex').")
            else:
                state.success("Voice listening active (hands-free mode).")
        else:
            state.warning("Voice loop disabled. Use text input box.")

    def stop(self):
        self._running = False
        self.capture.stop()
        bridge.stop()
        state.set_status(ZyrexStatus.OFFLINE)
        state.info("ZYREX stopped.")

    def process_text_command(self, text: str):
        """Called from UI text input — bypasses voice"""
        if not text.strip():
            return

        state.set_status(ZyrexStatus.PROCESSING)
        state.set_command(text)
        state.command(f'You: "{text}"')
        self._run_ai_pipeline(text, heard_prefix="You")

    def _execute(self, text: str):
        """Parse and execute a command from voice or text"""
        state.set_status(ZyrexStatus.PROCESSING)
        state.set_command(text)
        state.command(f'Heard: "{text}"')
        self._run_ai_pipeline(text, heard_prefix="Heard")

    def _run_ai_pipeline(self, text: str, heard_prefix: str = "Heard"):
        """voice/text -> AI planning -> command execution -> verification -> TTS"""
        state.command(f'{heard_prefix}: "{text}"')
        commands = ai.process(text)

        if text.lower().strip() in {"exit", "exit zyrex", "quit zyrex", "bye zyrex"}:
            self.stop()
            return

        if not commands:
            state.warning(f"AI produced no safe command for: {text}")
            ai.speak("I could not find a safe command for that request.")
            state.set_status(ZyrexStatus.IDLE)
            return

        response = ai.execute_and_respond(commands)
        if response:
            ai.speak(response)
            state.set_result(response)
        time.sleep(0.2)
        state.set_status(ZyrexStatus.IDLE)

    def _voice_loop(self):
        """Main continuous voice loop"""
        state.info("Voice loop started — speak anytime!")

        while self._running:
            try:
                # 1. Record until silence
                audio = self.capture.record_until_silence()

                if not self._running:
                    break

                if len(audio) == 0:
                    state.set_status(ZyrexStatus.IDLE)
                    time.sleep(0.1)
                    continue

                # 2. Transcribe
                state.set_status(ZyrexStatus.PROCESSING)
                state.info("Processing speech...")

                text = self.voice.transcribe(audio)

                if not text:
                    state.set_status(ZyrexStatus.IDLE)
                    continue

                lowered = text.lower().strip()
                if USE_WAKE_WORD:
                    wake_variants = ["hey zyrex", "hi zyrex", "ok zyrex"]
                    matched = next((w for w in wake_variants if w in lowered), "")
                    if matched:
                        idx = lowered.find(matched)
                        text = text[idx + len(matched):].strip() or "status"
                    else:
                        state.info("Wake word not detected.")
                        state.set_status(ZyrexStatus.IDLE)
                        continue

                # 3. Execute
                self._execute(text)

            except Exception as e:
                state.error(f"Voice loop error: {e}")
                state.set_status(ZyrexStatus.IDLE)
                time.sleep(1)


# ─────────────────────────────────────────────────────────────
#  SINGLETON
# ─────────────────────────────────────────────────────────────
brain = ZyrexBrain()

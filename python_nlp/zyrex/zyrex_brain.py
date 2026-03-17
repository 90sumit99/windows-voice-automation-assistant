"""
zyrex_brain.py
──────────────
Python brain for Zyrex.
Fixed: never crashes — UI stays open even if
whisper/sounddevice/C++ are not ready yet.
"""

import threading
import time
import re

from zyrex_state import state, ZyrexStatus, ModuleStatus, LogLevel
from zyrex_bridge import bridge


WHISPER_MODEL     = "base"
SAMPLE_RATE       = 16000
RECORD_SECONDS    = 5
SILENCE_THRESHOLD = 0.01


# ─────────────────────────────────────────────────────────────
#  SYSTEM STATS  (safe — psutil errors are caught)
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
                    stats = psutil.net_if_stats()
                    for iface, s in stats.items():
                        il = iface.lower()
                        if s.isup and ("wi" in il or "wlan" in il
                                       or "wireless" in il):
                            wifi = True
                            break
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

                state.set_system_stats(
                    cpu, ram, disk, wifi, battery, plugged)

            except ImportError:
                pass
            except Exception:
                pass

            time.sleep(2)


# ─────────────────────────────────────────────────────────────
#  VOICE ENGINE  (safe load)
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
            state.error(
                "faster-whisper not installed. "
                "Run: python -m pip install faster-whisper")
            return False
        except Exception as e:
            state.error(f"Whisper load failed: {e}")
            return False

    def transcribe(self, audio) -> str:
        if not self.is_loaded or self.model is None:
            return ""
        try:
            import numpy as np
            audio = audio.astype(np.float32)
            segs, _ = self.model.transcribe(
                audio, language="en",
                beam_size=3, vad_filter=True)
            return " ".join(s.text.strip() for s in segs).strip()
        except Exception as e:
            state.error(f"Transcription error: {e}")
            return ""


# ─────────────────────────────────────────────────────────────
#  AUDIO CAPTURE  (safe)
# ─────────────────────────────────────────────────────────────
class AudioCapture:
    def record(self, duration=RECORD_SECONDS):
        try:
            import numpy as np
            import sounddevice as sd
            state.set_status(ZyrexStatus.LISTENING)
            audio = sd.rec(
                int(duration * SAMPLE_RATE),
                samplerate=SAMPLE_RATE,
                channels=1, dtype="float32")
            sd.wait()
            flat = audio.flatten()

            # waveform
            chunk = max(1, len(flat) // 30)
            wv = [float(abs(flat[i*chunk:(i+1)*chunk]).mean())
                  for i in range(30)]
            state.set_waveform(wv)
            return flat

        except ImportError:
            state.error(
                "sounddevice not installed. "
                "Run: python -m pip install sounddevice")
            import numpy as np
            return np.array([])
        except Exception as e:
            state.error(f"Audio capture error: {e}")
            import numpy as np
            return np.array([])

    @staticmethod
    def has_speech(audio) -> bool:
        try:
            import numpy as np
            if len(audio) == 0:
                return False
            return float((audio**2).mean()**0.5) > SILENCE_THRESHOLD
        except Exception:
            return False


# ─────────────────────────────────────────────────────────────
#  INTENT PARSER
# ─────────────────────────────────────────────────────────────
class IntentParser:
    def parse(self, text: str) -> str:
        t = text.lower().strip()
        if not t:
            return ""

        if any(w in t for w in ["exit", "quit", "bye"]):
            return "exit"
        if any(w in t for w in ["shut down","shutdown","turn off"]):
            return "shutdown"
        if any(w in t for w in ["restart","reboot"]):
            return "restart"
        if "sleep" in t:
            return "sleep"
        if any(w in t for w in ["lock","lock screen"]):
            return "lock"
        if "unmute" in t:
            return "unmute"
        if "mute" in t:
            return "mute"
        if any(w in t for w in ["volume up","louder","turn up"]):
            return "volume up"
        if any(w in t for w in ["volume down","quieter","turn down"]):
            return "volume down"
        if "volume" in t:
            nums = re.findall(r'\d+', t)
            return f"volume {nums[0]}" if nums else "volume 50"
        if any(w in t for w in ["next song","next track","skip"]):
            return "next track"
        if any(w in t for w in ["previous song","prev song","go back"]):
            return "previous track"
        if any(w in t for w in ["play pause","pause","resume"]):
            return "play"
        if any(w in t for w in
               ["screenshot","capture screen","screen capture"]):
            return "screenshot"
        if any(w in t for w in ["find","search for","locate"]):
            q = t
            for n in ["find","search","for","locate","where","is","the","my"]:
                q = q.replace(n, "").strip()
            return f"find {q}"
        if "delete" in t:
            tgt = t.replace("delete","").replace("file","").strip()
            return f"delete {tgt}"
        if any(w in t for w in ["open file","open document"]):
            tgt = t
            for n in ["open","file","document","pdf","the","my"]:
                tgt = tgt.replace(n,"").strip()
            return f"openfile {tgt}"
        if any(w in t for w in ["close","kill","terminate"]):
            tgt = t
            for n in ["close","kill","terminate","app","application"]:
                tgt = tgt.replace(n,"").strip()
            return f"close {tgt}"
        if any(w in t for w in ["open","launch","start"]):
            tgt = t
            for n in ["open","launch","start","the","app","application"]:
                tgt = tgt.replace(n,"").strip()
            return f"open {tgt}"
        if any(w in t for w in ["ip address","my ip","ipconfig"]):
            return "ip"
        if "wifi" in t:
            return "wifi"

        return t   # send raw to C++ IntentResolver as fallback


# ─────────────────────────────────────────────────────────────
#  ZYREX BRAIN
# ─────────────────────────────────────────────────────────────
class ZyrexBrain:
    def __init__(self):
        self.voice   = VoiceEngine()
        self.audio   = AudioCapture()
        self.parser  = IntentParser()
        self._running = False

    def start(self):
        state.info("Starting Zyrex brain...")
        state.set_module("AI Brain", ModuleStatus.ONLINE)

        # System stats in background
        SystemStatsUpdater().start()

        # Try load whisper (non-fatal if fails)
        whisper_ok = self.voice.load()
        if not whisper_ok:
            state.warning(
                "Voice engine offline. "
                "Text commands still work.")

        # Try connect C++ bridge (non-fatal if fails)
        bridge_ok = bridge.start()
        if not bridge_ok:
            state.warning(
                "C++ bridge offline. "
                "Build VoiceAutomationCore.exe first.")

        # Mark system ready
        state.set_status(ZyrexStatus.IDLE)
        state.success("ZYREX ready. Type commands below.")

        # Only start listen loop if whisper loaded
        if whisper_ok:
            self._running = True
            t = threading.Thread(
                target=self._listen_loop, daemon=True)
            t.start()

    def stop(self):
        self._running = False
        bridge.stop()
        state.set_status(ZyrexStatus.OFFLINE)

    def process_text_command(self, text: str):
        """Called from UI text input box"""
        if not text.strip():
            return
        state.set_status(ZyrexStatus.PROCESSING)
        state.set_command(text)
        state.command(f'You typed: "{text}"')

        cmd = self.parser.parse(text)
        if cmd:
            state.set_status(ZyrexStatus.EXECUTING)
            bridge.send_command(cmd)
        else:
            state.warning(f"Could not parse: {text}")

        state.set_status(ZyrexStatus.IDLE)

    def _listen_loop(self):
        while self._running:
            try:
                audio = self.audio.record(RECORD_SECONDS)
                if not self.audio.has_speech(audio):
                    state.set_waveform([0.0] * 30)
                    time.sleep(0.1)
                    continue

                state.set_status(ZyrexStatus.PROCESSING)
                text = self.voice.transcribe(audio)

                if not text:
                    state.set_status(ZyrexStatus.IDLE)
                    continue

                state.command(f'Heard: "{text}"')
                state.set_command(text)
                state.set_status(ZyrexStatus.EXECUTING)

                cmd = self.parser.parse(text)
                if cmd == "exit":
                    self.stop()
                    break
                elif cmd:
                    bridge.send_command(cmd)
                else:
                    state.warning(f"Unknown: {text}")

                state.set_status(ZyrexStatus.IDLE)
                time.sleep(0.2)

            except Exception as e:
                state.error(f"Listen error: {e}")
                time.sleep(1)


# ─────────────────────────────────────────────────────────────
#  SINGLETON
# ─────────────────────────────────────────────────────────────
brain = ZyrexBrain()

"""
mic_test.py
───────────
Run this BEFORE starting Zyrex to confirm
your mic is detected and working.

Place in: python_nlp/zyrex/
Run:      python mic_test.py
"""

print("=" * 50)
print("  ZYREX MIC TEST")
print("=" * 50)

# ── Step 1: Check sounddevice ─────────────────────────
print("\n[1] Checking sounddevice...")
try:
    import sounddevice as sd
    print("    ✓ sounddevice installed")
except ImportError:
    print("    ✗ sounddevice NOT installed")
    print("    Fix: python -m pip install sounddevice")
    input("\nPress Enter to exit")
    exit()

# ── Step 2: List all mics ─────────────────────────────
print("\n[2] Available microphones:")
try:
    devices = sd.query_devices()
    input_devices = []
    for i, d in enumerate(devices):
        if d['max_input_channels'] > 0:
            input_devices.append((i, d['name']))
            print(f"    [{i}] {d['name']}")

    if not input_devices:
        print("    ✗ No microphones found!")
        print("    Check: Settings → Sound → Input")
        input("\nPress Enter to exit")
        exit()

    default = sd.query_devices(kind='input')
    print(f"\n    Default mic: {default['name']}")
    print(f"    Sample rate: {int(default['default_samplerate'])} Hz")
    print(f"    Channels:    {default['max_input_channels']}")
    print("    ✓ Microphone detected")

except Exception as e:
    print(f"    ✗ Error: {e}")
    input("\nPress Enter to exit")
    exit()

# ── Step 3: Record 3 seconds ──────────────────────────
print("\n[3] Recording 3 seconds of audio...")
print("    >>> SPEAK NOW <<<")

try:
    import numpy as np
    audio = sd.rec(
        int(3 * 16000),
        samplerate=16000,
        channels=1,
        dtype='float32'
    )
    sd.wait()
    flat = audio.flatten()
    rms  = float(np.sqrt(np.mean(flat ** 2)))

    print(f"    Recorded {len(flat)/16000:.1f} seconds")
    print(f"    Audio energy (RMS): {rms:.5f}")

    if rms < 0.001:
        print("    ⚠ Very low energy — mic might be muted or too quiet")
        print("    Fix: Windows Settings → Sound → Input → Volume slider up")
    elif rms < 0.005:
        print("    ⚠ Low energy — try speaking louder or moving mic closer")
        print(f"    Tip: Set SILENCE_THRESHOLD = 0.003 in zyrex_brain.py")
    else:
        print(f"    ✓ Good audio level detected!")
        print(f"    Tip: SILENCE_THRESHOLD = 0.008 should work fine")

except Exception as e:
    print(f"    ✗ Recording failed: {e}")
    input("\nPress Enter to exit")
    exit()

# ── Step 4: Check faster-whisper ─────────────────────
print("\n[4] Checking faster-whisper...")
try:
    from faster_whisper import WhisperModel
    print("    ✓ faster-whisper installed")
except ImportError:
    print("    ✗ faster-whisper NOT installed")
    print("    Fix: python -m pip install faster-whisper")
    input("\nPress Enter to exit")
    exit()

# ── Step 5: Quick transcription test ─────────────────
print("\n[5] Testing Whisper transcription...")
print("    Loading base model (first time may take 1-2 min)...")

try:
    import numpy as np

    model = WhisperModel("base", device="cpu", compute_type="int8")
    print("    ✓ Whisper model loaded!")

    print("\n    >>> Say something (5 seconds) <<<")
    audio2 = sd.rec(int(5*16000), samplerate=16000, channels=1, dtype='float32')
    sd.wait()

    print("    Transcribing...")
    segs, _ = model.transcribe(audio2.flatten(), language="en", beam_size=3)
    text = " ".join(s.text.strip() for s in segs).strip()

    if text:
        print(f"    ✓ Transcribed: \"{text}\"")
        print("\n    VOICE IS WORKING! You can now run Zyrex.")
    else:
        print("    ⚠ Nothing transcribed — try speaking louder")
        print("    Or lower SILENCE_THRESHOLD in zyrex_brain.py")

except Exception as e:
    print(f"    ✗ Whisper test failed: {e}")

print("\n" + "=" * 50)
print("  TEST COMPLETE")
print("=" * 50)
input("\nPress Enter to exit")

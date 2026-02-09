import sounddevice as sd
import queue
import json
from vosk import Model, KaldiRecognizer

# =========================
# CONFIG
# =========================
MODEL_PATH = "model/vosk-model-small-en-us"
MIC_INDEX = 1      # Change if needed
SAMPLE_RATE = 16000

# =========================
# INTENT LOGIC
# =========================
APP_KEYWORDS = {
    "notepad": ["notepad", "notes"],
    "calculator": ["calculator", "calc"],
    "paint": ["paint", "mspaint"]
}

MUTE_KEYWORDS = ["mute", "silence"]
UNMUTE_KEYWORDS = ["unmute", "sound on"]
TIME_KEYWORDS = ["time", "current time"]

def detect_intent(command: str):
    cmd = command.lower()

    if "open" in cmd:
        for app, words in APP_KEYWORDS.items():
            for w in words:
                if w in cmd:
                    return {
                        "intent": "OPEN_APP",
                        "target": app,
                        "confidence": 0.95,
                        "raw_text": command
                    }

    if any(w in cmd for w in MUTE_KEYWORDS):
        return {
            "intent": "VOLUME_MUTE",
            "target": None,
            "confidence": 0.90,
            "raw_text": command
        }

    if any(w in cmd for w in UNMUTE_KEYWORDS):
        return {
            "intent": "VOLUME_UNMUTE",
            "target": None,
            "confidence": 0.90,
            "raw_text": command
        }

    if any(w in cmd for w in TIME_KEYWORDS):
        return {
            "intent": "GET_TIME",
            "target": None,
            "confidence": 0.85,
            "raw_text": command
        }

    return {
        "intent": "UNKNOWN",
        "target": None,
        "confidence": 0.0,
        "raw_text": command
    }

# =========================
# AUDIO CALLBACK
# =========================
q = queue.Queue()

def callback(indata, frames, time, status):
    if status:
        pass  # Avoid printing non-JSON
    q.put(bytes(indata))

# =========================
# MAIN
# =========================
model = Model(MODEL_PATH)
recognizer = KaldiRecognizer(model, SAMPLE_RATE)

try:
    with sd.RawInputStream(
        samplerate=SAMPLE_RATE,
        blocksize=8000,
        dtype='int16',
        channels=1,
        device=MIC_INDEX,
        callback=callback
    ):
        print(json.dumps({
            "status": "MIC_OPEN",
            "message": "Microphone is active"
        }))

        while True:
            data = q.get()
            if recognizer.AcceptWaveform(data):
                result = json.loads(recognizer.Result())
                text = result.get("text", "").strip()

                if text:
                    intent_data = detect_intent(text)

                    # 🔴 VERY IMPORTANT: ONLY JSON OUTPUT
                    print(json.dumps(intent_data))

except KeyboardInterrupt:
    # ✅ Graceful Ctrl+C handling
    print(json.dumps({
        "status": "MIC_CLOSED",
        "message": "Microphone closed by user"
    }))

except Exception as e:
    # ✅ Any unexpected error
    print(json.dumps({
        "status": "ERROR",
        "message": str(e)
    }))

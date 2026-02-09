import sounddevice as sd
import queue
import json
from vosk import Model, KaldiRecognizer

MODEL_PATH = "model/vosk-model-small-en-us"
MIC_INDEX = 1  # 🎯 Kreo Sonik (MME)

q = queue.Queue()

def callback(indata, frames, time, status):
    if status:
        print(status)
    q.put(bytes(indata))

model = Model(MODEL_PATH)
recognizer = KaldiRecognizer(model, 16000)

with sd.RawInputStream(
    samplerate=16000,
    blocksize=8000,
    dtype='int16',
    channels=1,
    device=MIC_INDEX,
    callback=callback
):
    print("🎤 Speak now (Ctrl+C to stop)...")

    while True:
        data = q.get()
        if recognizer.AcceptWaveform(data):
            result = json.loads(recognizer.Result())
            text = result.get("text", "")
            if text:
                print("You said:", text)

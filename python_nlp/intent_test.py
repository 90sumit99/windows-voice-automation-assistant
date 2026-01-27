import json
import datetime

# -----------------------------
# Intent keywords (RULE BASED)
# -----------------------------
APP_KEYWORDS = {
    "notepad": ["notepad", "notes"],
    "calculator": ["calculator", "calc"],
    "paint": ["paint", "mspaint"]
}

MUTE_KEYWORDS = ["mute", "silence"]
UNMUTE_KEYWORDS = ["unmute", "sound on"]
TIME_KEYWORDS = ["time", "current time"]

# -----------------------------
# Intent Detection Logic
# -----------------------------
def detect_intent(command: str):
    command = command.lower()

    # OPEN APP
    for app, keywords in APP_KEYWORDS.items():
        for word in keywords:
            if word in command and "open" in command:
                return {
                    "intent": "OPEN_APP",
                    "target": app,
                    "confidence": 0.95
                }

    # MUTE
    if any(word in command for word in MUTE_KEYWORDS):
        return {
            "intent": "VOLUME_MUTE",
            "target": None,
            "confidence": 0.90
        }

    # UNMUTE
    if any(word in command for word in UNMUTE_KEYWORDS):
        return {
            "intent": "VOLUME_UNMUTE",
            "target": None,
            "confidence": 0.90
        }

    # TIME
    if any(word in command for word in TIME_KEYWORDS):
        current_time = datetime.datetime.now().strftime("%H:%M:%S")
        return {
            "intent": "GET_TIME",
            "target": current_time,
            "confidence": 0.85
        }

    # UNKNOWN
    return {
        "intent": "UNKNOWN",
        "target": None,
        "confidence": 0.0
    }

# -----------------------------
# MAIN ENTRY POINT
# -----------------------------
if __name__ == "__main__":
    user_command = input("Enter command: ")
    result = detect_intent(user_command)
    print(json.dumps(result))

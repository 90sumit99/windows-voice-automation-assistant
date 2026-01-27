# 🖥️ Offline Windows Voice Assistant – Python NLP Module

## 📌 Project Overview

This repository contains the **Python-based NLP and Offline Speech Recognition module** for a **Windows Voice Assistant**.

### 🎯 Purpose of this Module

- 🎤 Listen to microphone input **offline**
- 🗣️ Convert speech to text using **Vosk**
- 🧠 Detect user intent using **rule-based NLP**
- 📤 Output **structured JSON** for a **C++ Windows system controller**

> ⚠️ **Important**  
> This Python module **DOES NOT execute system commands**.  
> It only understands commands and outputs **safe JSON**.

---

## ❓ Why This Project?

Typical voice assistants rely on:

- 🌐 Internet
- ☁️ Cloud APIs
- 🔓 Privacy-invasive models

### ✅ Our Goal

To build a system that is:

- Fully **offline**
- **Secure**
- **Deterministic**
- **System-level compatible**
- **Easy to integrate with C++**

---

## 🧠 System Architecture
Microphone
↓
Offline Speech Recognition (Vosk)
↓
Text Processing
↓
Intent Detection (Rule-Based NLP)
↓
JSON Output (stdout)
↓
C++ Windows System Controller (future)


---

## 🧩 Technology Stack

| Component          | Technology                  |
| ------------------ | --------------------------- |
| Language           | Python 3.10 / 3.11          |
| Speech Recognition | Vosk (Offline)              |
| Audio Input        | sounddevice                 |
| NLP                | Rule-based intent detection |
| Output Format      | JSON                        |
| Target OS          | Windows                     |

---

## 📂 Folder Structure

python_nlp/
│
├── assistant.py # Main Python voice assistant
├── requirements.txt # Python dependencies
├── README.md # Project documentation
│
└── model/
└── vosk-model-small-en-us/

---

## 🔧 Installation & Setup (From Scratch)

### 1️⃣ Install Python

- Install **Python 3.10 or 3.11**
- ✅ Check **Add Python to PATH**

Verify installation:

```bash
python --version
```

## 🔧 Installation & Setup

### 2️⃣ Install Dependencies
```bash
pip install vosk sounddevice
```
### 3️⃣ Download Offline Vosk Model
Download: vosk-model-small-en-us
Extract to:
```bash
python_nlp/model/vosk-model-small-en-us
```
## ▶️ Running the Assistant
```bash
python assistant.py
```
```bash
{"status": "MIC_OPEN", "message": "Microphone is active"}

Speak commands:
- open calculator
- mute volume
- what is the time

Stop using Ctrl + C

## 🧾 JSON Output Format
{
  "intent": "OPEN_APP",
  "target": "calculator",
  "confidence": 0.95,
  "raw_text": "open calculator"
}
```
System Events:
MIC_OPEN
MIC_CLOSED
ERROR

## 🧠 Supported Intents
OPEN_APP – open calculator
VOLUME_MUTE – mute volume
VOLUME_UNMUTE – unmute volume
GET_TIME – what is the time
UNKNOWN – unsupported command

## ❗ Errors Faced & Fixes
PyAudio issue fixed by sounddevice
Mic callback issue fixed via queue
Ctrl+C crash fixed with try/except

## 🔐 Security Design
No system commands
JSON-only IPC

## 🏁 Status
Python NLP + Offline Voice Module Complete (v1 Frozen)

Author: Lavya Kumar


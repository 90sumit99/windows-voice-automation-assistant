Windows Offline Voice Automation System

A secure, fully offline voice-controlled automation system for Windows built using Python (Speech + NLP) and C++ (System-Level Execution via WinAPI).

This project demonstrates system-level programming, secure command handling, offline speech processing, and structured inter-process communication between Python and C++.

📌 Project Overview

This system is designed as a two-layer architecture:

1️⃣ Voice & NLP Layer (Python)

Listens to microphone input

Converts speech to text (offline)

Detects user intent using rule-based NLP

Outputs structured JSON

Does NOT execute system commands

2️⃣ Windows Automation Core (C++)

Receives structured JSON

Validates commands using whitelist-based security

Executes approved operations via WinAPI

Maintains execution logging

All system-level execution logic is implemented in C++.

🧑‍💻 The backend Windows automation core and system-level architecture were primarily developed by Sumit Chaudhary.

🎯 Why This Project?

Most voice assistants depend on:

Internet connectivity

Cloud APIs

Black-box AI models

Privacy-invasive processing

✅ Our Goal

To build a system that is:

Fully offline

Secure and deterministic

System-level compatible

Modular and extensible

Suitable for OS-level automation

🧠 System Architecture

Microphone
↓
Offline Speech Recognition (Vosk – Prototype)
↓
Text Processing
↓
Rule-Based Intent Detection
↓
Structured JSON Output
↓
C++ Windows Automation Core
↓
WinAPI Execution

The Python module only understands commands.
The C++ module controls the system securely.

🛠 Technology Stack
Python Voice & NLP Layer
Component	Technology
Language	Python 3.10 / 3.11
Speech Recognition	Vosk (Offline – Used for Testing/Prototype)
Audio Input	sounddevice
NLP	Rule-based intent detection
Output	JSON (stdout)
OS	Windows

⚠️ Vosk is currently used for development and testing purposes.
The long-term roadmap includes upgrading to a more advanced AI-based speech recognition and intent detection system.

C++ Windows Automation Core
Component	Technology
Language	C++17
OS	Windows 10 / 11
System API	WinAPI
Compiler	MSVC (Visual Studio)
Security	Whitelist-based command validation
Logging	File-based logging system
📂 Project Structure
root/
│
├── python_nlp/
│   ├── assistant.py
│   ├── requirements.txt
│   ├── README.md
│   └── model/
│       └── vosk-model-small-en-us/
│
├── VoiceAutomationCore/
│   ├── src/
│   ├── include/
│   ├── logs/
│   └── README.md
│
└── README.md

🚀 Current Features
Python Module

Offline microphone listening

Speech-to-text conversion

Rule-based intent detection

Structured JSON output

Clean exit handling

No system execution logic

C++ Core

Custom command parser

Whitelist-based security validation

Controlled WinAPI execution

Modular architecture

Execution logging

🧾 JSON Communication Format

Example:

{
  "intent": "OPEN_APP",
  "target": "calculator",
  "confidence": 0.95,
  "raw_text": "open calculator"
}


System Events:

MIC_OPEN

MIC_CLOSED

ERROR

This ensures structured and secure inter-process communication.

🔐 Security Design

No direct shell command execution

No arbitrary system calls

JSON-only IPC between Python and C++

Strict whitelist validation

Deterministic execution model

Fully offline processing

Security is enforced at the C++ system layer.

🧩 Supported Intents (v1)

OPEN_APP

VOLUME_MUTE

VOLUME_UNMUTE

GET_TIME

UNKNOWN

📈 Future Enhancements
🤖 AI-Based Command Understanding

Replace rule-based NLP with ML-based intent classification

Context-aware multi-step command processing

Transformer-based language models

🎙 Advanced Speech Engine

Replace Vosk prototype with scalable speech recognition

Improve accuracy and noise handling

Add multilingual support

🖥 Screen Visualization & Context Awareness

Real-time screen content analysis

Context-driven automation

Computer vision integration

🔄 Advanced Automation

Workflow chaining

Scheduled task automation

User-defined command extensions

💼 Professional Relevance

This project demonstrates:

System-level C++ programming

WinAPI usage

Secure software design

Modular architecture principles

Offline AI integration

Inter-process communication

Defensive programming practices

It reflects real-world engineering practices applicable to:

Systems Engineering

Backend Development

OS-Level Tooling

Automation Software

Security-Oriented Development

📌 Current Status

Python Voice + NLP Module: Complete (v1 – Stable Prototype)

C++ Windows Automation Core: Under Active Development

AI & Vision Extensions: Planned

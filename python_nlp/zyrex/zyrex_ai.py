"""
zyrex_ai.py
-----------
Gemini 1.5 Flash AI brain for ZYREX.
Replaces rule-based IntentParser with real AI.

Install: pip install google-generativeai pyttsx3
"""

from __future__ import annotations

import json
import os
import re
import time
from typing import Any

from auto_control import auto_control
from zyrex_bridge import bridge
from zyrex_state import ModuleStatus, ZyrexStatus, state

try:
    import pyttsx3
except Exception:  # pragma: no cover - optional dependency at runtime
    pyttsx3 = None

try:
    import google.generativeai as genai
except Exception:  # pragma: no cover - optional dependency at runtime
    genai = None


GEMINI_API_KEY = os.getenv("GEMINI_API_KEY", "YOUR_KEY_HERE")
GEMINI_CONFIG_FILE = "gemini_config.json"

SYSTEM_PROMPT = """
You are ZYREX, an AI Windows automation assistant.
The user will give you a task in natural language.
You must return ONLY a valid JSON object — no explanation, no markdown.

Return this exact format:
{
  "commands": ["cmd1", "cmd2"],
  "needs_admin": false,
  "explanation": "what these commands do",
  "response": "what to say to user after execution"
}

Rules:
- commands must be valid Windows CMD or PowerShell commands
- Use "internal:*" commands for local automation when needed:
  - internal:create_module:module_name
  - internal:find_file:filename_or_pattern
  - internal:create_file:relative/path.txt:content
  - internal:append_file:relative/path.txt:content
  - internal:read_file:relative/path.txt
  - internal:list_dir:relative/path
  - internal:mouse_move:x:y
  - internal:mouse_click:x:y
  - internal:mouse_double_click:x:y
  - internal:type_text:your_text_here
- For opening apps: use "start appname" or "start URL"
- For Spotify: use "start spotify:search:songname"
- For YouTube: use "start https://youtube.com/results?search_query=songname"
- NEVER suggest: format, del /s on C:\\, rm -rf, registry deletion
- needs_admin: true only if commands require elevation
- Keep response short and friendly — max 2 sentences
- If task is impossible on Windows, explain why in response
""".strip()

SIMPLE_COMMANDS = {
    "mute": ["mute"],
    "unmute": ["unmute"],
    "volume up": ["volume up"],
    "volume down": ["volume down"],
    "screenshot": ["screenshot"],
    "sleep": ["sleep"],
    "lock": ["lock"],
    "next song": ["next track"],
    "previous song": ["previous track"],
    "play pause": ["play"],
}

DANGEROUS_PATTERNS = [
    "format ",
    "del /s",
    "del /f",
    "rd /s",
    "remove-item -recurse",
    "rm -rf",
    "reg delete",
    "bcdedit /delete",
    "cipher /w",
]


class ZyrexAI:
    def __init__(self):
        self._model = None
        self._model_name = ""
        self._tts = None
        self.last_plan: dict[str, Any] = {}
        self.last_response: str = ""
        self.last_error: str = ""
        self._project_root = os.path.normpath(
            os.path.join(os.path.dirname(__file__), "..", "..")
        )
        self._config_path = os.path.join(os.path.dirname(__file__), GEMINI_CONFIG_FILE)

        self._load_saved_api_key()
        self._init_gemini()
        self._init_tts()
        state.set_module("AI Brain", ModuleStatus.ONLINE)

    def _load_saved_api_key(self):
        global GEMINI_API_KEY
        try:
            if os.path.exists(self._config_path):
                with open(self._config_path, "r", encoding="utf-8") as f:
                    data = json.load(f)
                saved = str(data.get("api_key", "")).strip()
                if saved:
                    GEMINI_API_KEY = saved
                    os.environ["GEMINI_API_KEY"] = saved
        except Exception as e:
            state.warning(f"Could not load saved Gemini key: {e}")

    def _init_gemini(self):
        if genai is None:
            self.last_error = "google-generativeai not installed."
            state.warning("Gemini SDK missing: pip install google-generativeai")
            return
        if not GEMINI_API_KEY or GEMINI_API_KEY == "YOUR_KEY_HERE":
            self.last_error = "Gemini API key not configured."
            state.warning("Set GEMINI_API_KEY env var or update zyrex_ai.py")
            return
        try:
            genai.configure(api_key=GEMINI_API_KEY)
            preferred_models = [
                "models/gemini-2.5-flash",
                "models/gemini-2.0-flash",
                "models/gemini-flash-latest",
            ]
            available = {
                m.name
                for m in genai.list_models()
                if "generateContent" in getattr(m, "supported_generation_methods", [])
            }
            chosen = next((name for name in preferred_models if name in available), "")
            if not chosen:
                raise RuntimeError("No compatible Gemini text model available for generateContent.")
            self._model = genai.GenerativeModel(chosen)
            self._model_name = chosen
            state.success(f"Gemini connected: {chosen}")
        except Exception as e:
            self.last_error = f"Gemini init failed: {e}"
            state.error(self.last_error)

    def set_api_key(self, api_key: str) -> bool:
        """Set Gemini key at runtime and reconnect model."""
        global GEMINI_API_KEY
        key = (api_key or "").strip()
        if not key:
            self.last_error = "Empty API key."
            return False
        GEMINI_API_KEY = key
        os.environ["GEMINI_API_KEY"] = key
        self._save_api_key(key)
        self._model = None
        self._init_gemini()
        return self._model is not None

    def _save_api_key(self, api_key: str):
        try:
            with open(self._config_path, "w", encoding="utf-8") as f:
                json.dump({"api_key": api_key}, f, indent=2)
        except Exception as e:
            state.warning(f"Could not save Gemini key: {e}")

    def _init_tts(self):
        if pyttsx3 is None:
            state.warning("pyttsx3 not installed, voice response disabled.")
            return
        try:
            self._tts = pyttsx3.init()
            self._tts.setProperty("rate", 175)
            self._tts.setProperty("volume", 0.95)
            state.success("TTS engine ready.")
        except Exception as e:
            state.warning(f"TTS init failed: {e}")
            self._tts = None

    def process(self, text: str) -> list[str]:
        """
        Generate safe command list for a natural-language user request.
        """
        try:
            state.set_status(ZyrexStatus.PROCESSING)
            state.set_command(text)
            state.info("THINKING...")
            self.last_error = ""
            self.last_response = ""

            local_commands = self._local_autonomous_commands(text)
            if local_commands:
                self.last_plan = {
                    "commands": local_commands,
                    "needs_admin": False,
                    "explanation": "Handled by local autonomous tools.",
                    "response": "Done.",
                }
                return local_commands

            simple = self._is_simple(text)
            if simple is not None:
                self.last_plan = {
                    "commands": simple,
                    "needs_admin": False,
                    "explanation": "Handled with built-in fast command mapping.",
                    "response": "Done.",
                }
                state.info("Simple command detected (fast path).")
                return simple

            plan = self._call_gemini(text)
            cmds = self._validate_commands(plan.get("commands", []))

            self.last_plan = {
                "commands": cmds,
                "needs_admin": bool(plan.get("needs_admin", False)),
                "explanation": str(plan.get("explanation", "")),
                "response": str(plan.get("response", "")).strip(),
            }
            return cmds
        except Exception as e:
            self.last_error = f"AI process error: {e}"
            state.error(self.last_error)
            return []

    def _local_autonomous_commands(self, text: str) -> list[str]:
        t = text.lower().strip()
        if "create" in t and "module" in t:
            m = re.search(r"(spotify|gmail|discord|whatsapp|screen_vision|auto_control|web_auto)", t)
            if m:
                return [f"internal:create_module:{m.group(1)}"]

        cf = re.search(r"create file\s+([a-zA-Z0-9_./\\-]+)", text, re.IGNORECASE)
        if cf:
            rel = cf.group(1).replace("\\", "/")
            return [f"internal:create_file:{rel}:# Auto-created by ZYREX\n"]

        ff = re.search(r"(find|search) file\s+(.+)", text, re.IGNORECASE)
        if ff:
            pat = ff.group(2).strip()
            return [f"internal:find_file:{pat}"]

        mm = re.search(r"move mouse to\s+(\d+)\s+(\d+)", t)
        if mm:
            return [f"internal:mouse_move:{mm.group(1)}:{mm.group(2)}"]

        mc = re.search(r"(click|left click)\s+(\d+)\s+(\d+)", t)
        if mc:
            return [f"internal:mouse_click:{mc.group(2)}:{mc.group(3)}"]

        dc = re.search(r"(double click)\s+(\d+)\s+(\d+)", t)
        if dc:
            return [f"internal:mouse_double_click:{dc.group(2)}:{dc.group(3)}"]

        if t.startswith("type "):
            payload = text[5:].strip()
            if payload:
                return [f"internal:type_text:{payload}"]
        return []

    def speak(self, text: str):
        if not text:
            return
        state.info(f"Zyrex: {text}")
        if self._tts is None:
            return
        try:
            self._tts.say(text)
            self._tts.runAndWait()
        except Exception as e:
            state.warning(f"TTS speak error: {e}")

    def _is_simple(self, text: str) -> list[str] | None:
        t = text.lower().strip()
        for key, cmd in SIMPLE_COMMANDS.items():
            if key in t:
                return cmd
        return None

    def _call_gemini(self, text: str) -> dict:
        fallback = {
            "commands": [],
            "needs_admin": False,
            "explanation": "AI unavailable or failed.",
            "response": "I could not process that right now.",
        }
        if self._model is None:
            state.warning("Gemini unavailable; returning safe fallback.")
            return fallback
        try:
            prompt = (
                f"{SYSTEM_PROMPT}\n\n"
                f'User task: "{text}"\n\n'
                "Return JSON only."
            )
            resp = self._model.generate_content(prompt)
            raw = (getattr(resp, "text", "") or "").strip()
            data = self._parse_json(raw)
            if not isinstance(data, dict):
                return fallback
            if not isinstance(data.get("commands", []), list):
                data["commands"] = []
            return data
        except Exception as e:
            self.last_error = f"Gemini call failed: {e}"
            state.error(self.last_error)
            return fallback

    def _parse_json(self, raw: str) -> dict[str, Any]:
        if not raw:
            return {}
        cleaned = raw.strip()
        if "```" in cleaned:
            cleaned = cleaned.replace("```json", "").replace("```", "").strip()
        start = cleaned.find("{")
        end = cleaned.rfind("}")
        if start != -1 and end != -1 and end > start:
            cleaned = cleaned[start:end + 1]
        try:
            return json.loads(cleaned)
        except Exception:
            state.warning("Gemini returned invalid JSON.")
            return {}

    def _validate_commands(self, commands: list) -> list[str]:
        safe: list[str] = []
        for cmd in commands:
            if not isinstance(cmd, str):
                continue
            c = cmd.strip()
            lower = c.lower()
            if not c:
                continue
            if lower.startswith("internal:"):
                safe.append(c)
                continue
            if any(pat in lower for pat in DANGEROUS_PATTERNS):
                state.warning(f"Blocked unsafe AI command: {c}")
                continue
            safe.append(self._normalize_bridge_command(c))
        if not safe:
            state.warning("No safe commands produced by AI.")
        return safe

    def _normalize_bridge_command(self, command: str) -> str:
        """
        Normalize Gemini output into command strings that C++ IntentResolver handles well.
        """
        c = command.strip()
        lower = c.lower()

        # Convert direct executable name to open intent
        if lower.endswith(".exe") and " " not in lower:
            return f"open {c[:-4]}"

        # Convert bare app token (e.g., "notepad") to open intent
        if re.fullmatch(r"[a-zA-Z0-9._-]+", c) and " " not in c:
            if c.lower() not in {"mute", "unmute", "sleep", "lock", "screenshot"}:
                return f"open {c}"

        # Normalize common media aliases to resolver words
        media_map = {
            "next": "next track",
            "previous": "previous track",
            "pause": "play",
            "play pause": "play",
        }
        if lower in media_map:
            return media_map[lower]

        return c

    def _execute_and_verify(self, commands: list[str], explanation: str) -> str:
        """
        Execute commands via bridge, then ask Gemini to verify based on output.
        """
        if not commands:
            return self.last_plan.get("response", "I could not find a safe command to run.")

        output_lines: list[str] = []
        for command in commands:
            state.set_status(ZyrexStatus.EXECUTING)
            state.info(f"Executing: {command}")
            if command.lower().startswith("internal:"):
                output_lines.append(self._execute_internal(command))
            else:
                bridge.send_command(command)
                time.sleep(0.35)
                output_lines.extend(bridge.drain_output(max_lines=40))

        raw_output = "\n".join(output_lines)[-6000:]

        # If model is unavailable, return basic response
        if self._model is None:
            return self.last_plan.get("response", "Done.")

        try:
            verify_prompt = (
                "You are ZYREX. Based on executed commands and terminal output, "
                "respond in max 2 sentences for user.\n\n"
                f"Command explanation: {explanation}\n"
                f"Commands: {commands}\n\n"
                f"Execution output:\n{raw_output}\n\n"
                "Return plain text only."
            )
            resp = self._model.generate_content(verify_prompt)
            text = (getattr(resp, "text", "") or "").strip()
            if text:
                return text
        except Exception as e:
            state.warning(f"Gemini verify failed: {e}")
        return self.last_plan.get("response", "Commands completed.")

    def _execute_internal(self, command: str) -> str:
        """Execute internal automation commands without C++ bridge."""
        parts = command.split(":")
        if len(parts) < 2:
            return "Invalid internal command."
        action = parts[1].strip().lower()
        try:
            if action == "create_module" and len(parts) >= 3:
                name = parts[2].strip().lower()
                return self._create_module_file(name)
            if action == "find_file" and len(parts) >= 3:
                return self._find_files(":".join(parts[2:]))
            if action == "create_file" and len(parts) >= 4:
                rel_path = parts[2]
                content = ":".join(parts[3:])
                return self._create_file(rel_path, content)
            if action == "append_file" and len(parts) >= 4:
                rel_path = parts[2]
                content = ":".join(parts[3:])
                return self._append_file(rel_path, content)
            if action == "read_file" and len(parts) >= 3:
                return self._read_file(parts[2])
            if action == "list_dir" and len(parts) >= 3:
                return self._list_dir(parts[2])
            if action == "mouse_move" and len(parts) >= 4:
                return auto_control.move_to(int(parts[2]), int(parts[3]))
            if action == "mouse_click" and len(parts) >= 4:
                return auto_control.click(int(parts[2]), int(parts[3]))
            if action == "mouse_double_click" and len(parts) >= 4:
                return auto_control.double_click(int(parts[2]), int(parts[3]))
            if action == "type_text" and len(parts) >= 3:
                return auto_control.type_text(":".join(parts[2:]))
            return f"Unknown internal action: {action}"
        except Exception as e:
            return f"Internal action failed: {e}"

    def _create_module_file(self, module_name: str) -> str:
        safe_name = re.sub(r"[^a-z0-9_]", "", module_name.lower())
        if not safe_name:
            return "Invalid module name."
        base = os.path.dirname(__file__)
        file_path = os.path.join(base, f"{safe_name}.py")
        if os.path.exists(file_path):
            return f"Module already exists: {safe_name}.py"

        template = (
            '"""\n'
            f"{safe_name}.py\n"
            "Auto-generated by ZYREX.\n"
            '"""\n\n'
            f"class {''.join(x.capitalize() for x in safe_name.split('_'))}Module:\n"
            "    def __init__(self):\n"
            "        # TODO: implement module setup.\n"
            "        self.ready = False\n\n"
            "    def run(self, *args, **kwargs):\n"
            '        return "TODO: implement module behavior."\n'
        )
        with open(file_path, "w", encoding="utf-8") as f:
            f.write(template)
        state.success(f"Auto-created module file: {safe_name}.py")
        return f"Created module file: {safe_name}.py"

    def _safe_path(self, rel_path: str) -> str:
        cleaned = rel_path.strip().replace("\\", "/").lstrip("/")
        full = os.path.normpath(os.path.join(self._project_root, cleaned))
        if not full.startswith(self._project_root):
            raise ValueError("Path escapes project root.")
        return full

    def _create_file(self, rel_path: str, content: str) -> str:
        full = self._safe_path(rel_path)
        folder = os.path.dirname(full)
        if folder and not os.path.exists(folder):
            os.makedirs(folder, exist_ok=True)
        if os.path.exists(full):
            return f"File already exists: {rel_path}"
        with open(full, "w", encoding="utf-8") as f:
            f.write(content)
        state.success(f"Created file: {rel_path}")
        return f"Created file: {rel_path}"

    def _append_file(self, rel_path: str, content: str) -> str:
        full = self._safe_path(rel_path)
        folder = os.path.dirname(full)
        if folder and not os.path.exists(folder):
            os.makedirs(folder, exist_ok=True)
        with open(full, "a", encoding="utf-8") as f:
            f.write(content)
        state.success(f"Updated file: {rel_path}")
        return f"Appended to file: {rel_path}"

    def _read_file(self, rel_path: str) -> str:
        full = self._safe_path(rel_path)
        if not os.path.exists(full):
            return f"File not found: {rel_path}"
        with open(full, "r", encoding="utf-8", errors="replace") as f:
            data = f.read(1200)
        return f"Read file {rel_path}: {data}"

    def _list_dir(self, rel_path: str) -> str:
        full = self._safe_path(rel_path or ".")
        if not os.path.isdir(full):
            return f"Directory not found: {rel_path}"
        items = sorted(os.listdir(full))[:80]
        return "Directory items: " + ", ".join(items)

    def _find_files(self, pattern: str) -> str:
        pat = (pattern or "").strip().lower()
        if not pat:
            return "No pattern provided."
        results: list[str] = []
        for root, _, files in os.walk(self._project_root):
            for name in files:
                if pat in name.lower():
                    rel = os.path.relpath(os.path.join(root, name), self._project_root)
                    results.append(rel.replace("\\", "/"))
                    if len(results) >= 20:
                        return "Found files: " + ", ".join(results)
        if not results:
            return f"No files matched: {pattern}"
        return "Found files: " + ", ".join(results)

    def execute_and_respond(self, commands: list[str]) -> str:
        explanation = self.last_plan.get("explanation", "")
        response = self._execute_and_verify(commands, explanation)
        self.last_response = response
        return response

    def ai_status(self) -> dict[str, Any]:
        return {
            "model_ready": self._model is not None,
            "model_name": self._model_name,
            "tts_ready": self._tts is not None,
            "auto_control_ready": auto_control.ready,
            "last_error": self.last_error,
            "last_response": self.last_response,
            "last_plan": self.last_plan,
        }


ai = ZyrexAI()

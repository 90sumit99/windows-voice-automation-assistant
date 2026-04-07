"""
zyrex_bridge.py
───────────────
Connects Python brain to your C++ VoiceAutomationCore.exe.
Sends text commands via stdin pipe.
Reads C++ stdout and forwards to ZyrexState log.

Place this in: WindowsVoiceAutomation/python_nlp/

IMPORTANT: Update CPP_EXE_PATH to point to your compiled .exe
"""

import subprocess
import threading
import os
import queue
from zyrex_state import state, ModuleStatus, LogLevel, ZyrexStatus


# ─────────────────────────────────────────────────────────────
#  PATH TO YOUR COMPILED C++ EXE
#  Change this to match your actual build output path
# ─────────────────────────────────────────────────────────────
CPP_EXE_PATH = os.path.join(
    os.path.dirname(__file__),   # python_nlp/zyrex/
    "..",                        # python_nlp/
    "..",                        # WindowsVoiceAutomation/
    "VoiceAutomationCore",
    "x64", "Debug",
    "VoiceAutomationCore.exe"
)


class ZyrexBridge:
    def __init__(self):
        self._process:   subprocess.Popen = None
        self._running:   bool             = False
        self._read_thread: threading.Thread = None
        self._result_queue: queue.Queue   = queue.Queue()

    # ─────────────────────────────────────────────────────────
    #  START — launch C++ exe as subprocess
    # ─────────────────────────────────────────────────────────
    def start(self) -> bool:
        exe = os.path.normpath(CPP_EXE_PATH)

        if not os.path.exists(exe):
            state.error(f"C++ exe not found: {exe}")
            state.set_module("C++ Bridge", ModuleStatus.DISCONNECTED)
            return False

        try:
            self._process = subprocess.Popen(
                [exe],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                encoding="utf-8",
                errors="replace",
                bufsize=1,                  # line buffered
            )
            self._running = True

            # Start background thread to read C++ output
            self._read_thread = threading.Thread(
                target=self._read_output,
                daemon=True
            )
            self._read_thread.start()

            state.set_module("C++ Bridge", ModuleStatus.CONNECTED)
            state.success("C++ VoiceAutomationCore connected.")

            # Set all action modules online
            for mod in ["Audio Controller", "File System",
                        "App Launcher", "Screenshot", "Music Controller"]:
                state.set_module(mod, ModuleStatus.ONLINE)

            return True

        except Exception as e:
            state.error(f"Failed to start C++ engine: {e}")
            state.set_module("C++ Bridge", ModuleStatus.DISCONNECTED)
            return False

    # ─────────────────────────────────────────────────────────
    #  SEND COMMAND — writes a line to C++ stdin
    # ─────────────────────────────────────────────────────────
    def send_command(self, command: str) -> bool:
        if not self._running or not self._process:
            state.error("C++ bridge not connected.")
            return False

        try:
            self._process.stdin.write(command + "\n")
            self._process.stdin.flush()
            state.command(f"→ C++: {command}")
            return True
        except BrokenPipeError:
            state.error("C++ bridge pipe broken. Reconnecting...")
            self._running = False
            state.set_module("C++ Bridge", ModuleStatus.DISCONNECTED)
            return False
        except Exception as e:
            state.error(f"Send failed: {e}")
            return False

    # ─────────────────────────────────────────────────────────
    #  READ OUTPUT — background thread reads C++ stdout
    # ─────────────────────────────────────────────────────────
    def _read_output(self):
        try:
            for line in self._process.stdout:
                line = line.strip()
                if not line:
                    continue

                # Parse C++ log output and forward to ZyrexState
                if "[SUCCESS]" in line or "SUCCESS" in line:
                    state.success(f"← {line}")
                elif "[ERROR]" in line or "ERROR" in line or "Failed" in line:
                    state.error(f"← {line}")
                elif "[WARNING]" in line or "WARNING" in line:
                    state.warning(f"← {line}")
                elif "ZYREX" in line and "started" in line.lower():
                    state.success("C++ Core ready.")
                else:
                    state.info(f"← {line}")
                self._result_queue.put(line)

        except Exception as e:
            state.error(f"C++ output read error: {e}")
        finally:
            self._running = False
            state.set_module("C++ Bridge", ModuleStatus.DISCONNECTED)
            state.warning("C++ engine disconnected.")

    # ─────────────────────────────────────────────────────────
    #  IS ALIVE
    # ─────────────────────────────────────────────────────────
    def is_connected(self) -> bool:
        return self._running and self._process is not None

    def drain_output(self, max_lines: int = 60) -> list[str]:
        """Drain recent C++ output lines from internal queue."""
        lines = []
        while len(lines) < max_lines:
            try:
                lines.append(self._result_queue.get_nowait())
            except queue.Empty:
                break
        return lines

    # ─────────────────────────────────────────────────────────
    #  STOP
    # ─────────────────────────────────────────────────────────
    def stop(self):
        if self._process:
            try:
                self.send_command("exit")
                self._process.wait(timeout=3)
            except Exception:
                self._process.kill()
        self._running = False
        state.set_module("C++ Bridge", ModuleStatus.DISCONNECTED)
        state.info("C++ bridge stopped.")


# ─────────────────────────────────────────────────────────────
#  GLOBAL SINGLETON
#  from zyrex_bridge import bridge
# ─────────────────────────────────────────────────────────────
bridge = ZyrexBridge()

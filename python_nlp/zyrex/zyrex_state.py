"""
zyrex_state.py
──────────────
Shared state for the entire Zyrex system.
UI reads this every 100ms.
Brain writes to this on every action.
Thread-safe via threading.Lock.

Place this in: WindowsVoiceAutomation/python_nlp/
"""

import threading
import time
from enum import Enum
from dataclasses import dataclass
from typing import List, Dict


# ─────────────────────────────────────────────────────────────
#  ENUMS
# ─────────────────────────────────────────────────────────────

class ZyrexStatus(Enum):
    IDLE        = "IDLE"
    LISTENING   = "LISTENING"
    PROCESSING  = "PROCESSING"
    EXECUTING   = "EXECUTING"
    ERROR       = "ERROR"
    OFFLINE     = "OFFLINE"

class ModuleStatus(Enum):
    ONLINE       = "ONLINE"
    OFFLINE      = "OFFLINE"
    READY        = "READY"
    CONNECTED    = "CONNECTED"
    DISCONNECTED = "DISCONNECTED"
    ACTIVE       = "ACTIVE"

class LogLevel(Enum):
    INFO    = "INFO"
    SUCCESS = "SUCCESS"
    WARNING = "WARNING"
    ERROR   = "ERROR"
    COMMAND = "COMMAND"


# ─────────────────────────────────────────────────────────────
#  LOG ENTRY
# ─────────────────────────────────────────────────────────────

@dataclass
class LogEntry:
    timestamp: str
    level:     LogLevel
    message:   str

    @staticmethod
    def now(level: LogLevel, message: str) -> "LogEntry":
        return LogEntry(
            timestamp=time.strftime("%H:%M:%S"),
            level=level,
            message=message
        )


# ─────────────────────────────────────────────────────────────
#  ZYREX STATE
# ─────────────────────────────────────────────────────────────

class ZyrexState:
    MAX_LOG = 200

    def __init__(self):
        self._lock = threading.Lock()

        # Status
        self.status:          ZyrexStatus = ZyrexStatus.OFFLINE
        self.current_command: str         = ""
        self.last_result:     str         = ""
        self.volume_level:    int         = 50
        self.is_muted:        bool        = False

        # Module statuses
        self.modules: Dict[str, ModuleStatus] = {
            "Voice Engine":     ModuleStatus.OFFLINE,
            "AI Brain":         ModuleStatus.OFFLINE,
            "Audio Controller": ModuleStatus.OFFLINE,
            "File System":      ModuleStatus.OFFLINE,
            "App Launcher":     ModuleStatus.OFFLINE,
            "Policy Engine":    ModuleStatus.ACTIVE,
            "Screenshot":       ModuleStatus.READY,
            "Music Controller": ModuleStatus.READY,
            "C++ Bridge":       ModuleStatus.DISCONNECTED,
        }

        # Log feed
        self.log_entries: List[LogEntry] = []

        # System stats
        self.cpu_percent:     float = 0.0
        self.ram_percent:     float = 0.0
        self.disk_percent:    float = 0.0
        self.wifi_connected:  bool  = False
        self.battery_percent: int   = 100
        self.battery_plugged: bool  = True

        # Waveform (30 bars)
        self.waveform_data: List[float] = [0.0] * 30

        self.log(LogLevel.INFO, "ZYREX initializing...")

    # ── Setters ───────────────────────────────────────────────

    def set_status(self, status: ZyrexStatus):
        with self._lock:
            self.status = status

    def set_command(self, command: str):
        with self._lock:
            self.current_command = command

    def set_result(self, result: str):
        with self._lock:
            self.last_result = result

    def set_module(self, name: str, status: ModuleStatus):
        with self._lock:
            self.modules[name] = status

    def set_volume(self, level: int, muted: bool = False):
        with self._lock:
            self.volume_level = max(0, min(100, level))
            self.is_muted = muted

    def set_waveform(self, data: List[float]):
        with self._lock:
            self.waveform_data = (data + [0.0] * 30)[:30]

    def set_system_stats(self, cpu: float, ram: float,
                         disk: float, wifi: bool,
                         battery: int, plugged: bool):
        with self._lock:
            self.cpu_percent     = cpu
            self.ram_percent     = ram
            self.disk_percent    = disk
            self.wifi_connected  = wifi
            self.battery_percent = battery
            self.battery_plugged = plugged

    # ── Logging ───────────────────────────────────────────────

    def log(self, level: LogLevel, message: str):
        with self._lock:
            self.log_entries.append(LogEntry.now(level, message))
            if len(self.log_entries) > self.MAX_LOG:
                self.log_entries = self.log_entries[-self.MAX_LOG:]

    def info   (self, msg: str): self.log(LogLevel.INFO,    msg)
    def success(self, msg: str): self.log(LogLevel.SUCCESS, msg)
    def warning(self, msg: str): self.log(LogLevel.WARNING, msg)
    def error  (self, msg: str): self.log(LogLevel.ERROR,   msg)
    def command(self, msg: str): self.log(LogLevel.COMMAND, msg)

    # ── Snapshot (UI calls this every 100ms) ──────────────────

    def snapshot(self) -> dict:
        with self._lock:
            return {
                "status":          self.status,
                "current_command": self.current_command,
                "last_result":     self.last_result,
                "is_muted":        self.is_muted,
                "volume_level":    self.volume_level,
                "modules":         dict(self.modules),
                "log_entries":     list(self.log_entries),
                "cpu":             self.cpu_percent,
                "ram":             self.ram_percent,
                "disk":            self.disk_percent,
                "wifi":            self.wifi_connected,
                "battery":         self.battery_percent,
                "battery_plugged": self.battery_plugged,
                "waveform":        list(self.waveform_data),
            }


# ─────────────────────────────────────────────────────────────
#  GLOBAL SINGLETON
#  from zyrex_state import state
# ─────────────────────────────────────────────────────────────
state = ZyrexState()

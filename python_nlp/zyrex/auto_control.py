"""
auto_control.py
Mouse + keyboard automation helpers for ZYREX.
"""

from __future__ import annotations

try:
    import pyautogui
except Exception:  # pragma: no cover
    pyautogui = None


class AutoControl:
    def __init__(self):
        self.ready = pyautogui is not None
        if self.ready:
            pyautogui.FAILSAFE = True
            pyautogui.PAUSE = 0.08

    def _clamp(self, x: int, y: int) -> tuple[int, int]:
        if not self.ready:
            return x, y
        w, h = pyautogui.size()
        return max(0, min(x, w - 1)), max(0, min(y, h - 1))

    def move_to(self, x: int, y: int, duration: float = 0.2) -> str:
        if not self.ready:
            return "Auto-control unavailable (install pyautogui)."
        x, y = self._clamp(int(x), int(y))
        pyautogui.moveTo(x, y, duration=max(0.0, float(duration)))
        return f"Mouse moved to ({x}, {y})."

    def click(self, x: int | None = None, y: int | None = None) -> str:
        if not self.ready:
            return "Auto-control unavailable (install pyautogui)."
        if x is not None and y is not None:
            x, y = self._clamp(int(x), int(y))
            pyautogui.click(x=x, y=y)
            return f"Clicked at ({x}, {y})."
        pyautogui.click()
        return "Clicked at current mouse position."

    def double_click(self, x: int | None = None, y: int | None = None) -> str:
        if not self.ready:
            return "Auto-control unavailable (install pyautogui)."
        if x is not None and y is not None:
            x, y = self._clamp(int(x), int(y))
            pyautogui.doubleClick(x=x, y=y)
            return f"Double-clicked at ({x}, {y})."
        pyautogui.doubleClick()
        return "Double-clicked at current mouse position."

    def type_text(self, text: str) -> str:
        if not self.ready:
            return "Auto-control unavailable (install pyautogui)."
        pyautogui.write(text, interval=0.02)
        return f"Typed {len(text)} characters."

    def hotkey(self, *keys: str) -> str:
        if not self.ready:
            return "Auto-control unavailable (install pyautogui)."
        if not keys:
            return "No hotkey provided."
        pyautogui.hotkey(*keys)
        return f"Pressed hotkey: {'+'.join(keys)}"


auto_control = AutoControl()

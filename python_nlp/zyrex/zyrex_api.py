"""
zyrex_api.py
────────────
Flask API server — the bridge between Electron UI and Zyrex brain.
Electron fetches /state every 200ms to get live data.
Electron posts to /command to send user commands.

Place in: WindowsVoiceAutomation/python_nlp/zyrex/
Run:      python zyrex_api.py
          (started automatically by Electron main.js)

Install:  pip install flask flask-cors
"""

from flask import Flask, jsonify, request
from flask_cors import CORS
import threading

from zyrex_state import state, ZyrexStatus, ModuleStatus, LogLevel
from zyrex_brain import brain

app = Flask(__name__)
CORS(app)  # allow Electron to call this API


# ─────────────────────────────────────────────────────────────
#  GET STATE  — Electron polls this every 200ms
# ─────────────────────────────────────────────────────────────
@app.route('/state', methods=['GET'])
def get_state():
    snap = state.snapshot()

    # Convert enums to strings for JSON
    return jsonify({
        'status':          snap['status'].value,
        'current_command': snap['current_command'],
        'last_result':     snap['last_result'],
        'is_muted':        snap['is_muted'],
        'volume_level':    snap['volume_level'],
        'modules': {
            name: s.value
            for name, s in snap['modules'].items()
        },
        'log_entries': [
            {
                'timestamp': e.timestamp,
                'level':     e.level.value,
                'message':   e.message
            }
            for e in snap['log_entries'][-100:]
        ],
        'cpu':             snap['cpu'],
        'ram':             snap['ram'],
        'disk':            snap['disk'],
        'wifi':            snap['wifi'],
        'battery':         snap['battery'],
        'battery_plugged': snap['battery_plugged'],
        'waveform':        snap['waveform'],
    })


# ─────────────────────────────────────────────────────────────
#  POST COMMAND  — Electron sends typed/voice commands here
# ─────────────────────────────────────────────────────────────
@app.route('/command', methods=['POST'])
def post_command():
    data = request.get_json()
    if not data or 'command' not in data:
        return jsonify({'error': 'No command provided'}), 400

    command = data['command'].strip()
    if not command:
        return jsonify({'error': 'Empty command'}), 400

    # Process in background so API returns immediately
    threading.Thread(
        target=brain.process_text_command,
        args=(command,),
        daemon=True
    ).start()

    return jsonify({'status': 'ok', 'command': command})


# ─────────────────────────────────────────────────────────────
#  STATUS CHECK  — Electron polls this to know API is ready
# ─────────────────────────────────────────────────────────────
@app.route('/status', methods=['GET'])
def status_check():
    return jsonify({'status': 'running', 'version': '1.0'})


# ─────────────────────────────────────────────────────────────
#  MAIN
# ─────────────────────────────────────────────────────────────
def start_brain_safe():
    try:
        brain.start()
    except Exception as e:
        state.error(f"Brain startup error: {e}")
        state.set_status(ZyrexStatus.IDLE)


if __name__ == '__main__':
    state.info("Starting Zyrex API server...")

    # Start brain in background
    threading.Thread(target=start_brain_safe, daemon=True).start()

    state.success("API server running on http://localhost:5000")

    # Run Flask (threaded for concurrent requests)
    app.run(
        host='127.0.0.1',
        port=5000,
        threaded=True,
        debug=False,
        use_reloader=False
    )

"""
zyrex_api.py — Flask API for Zyrex Electron UI
"""

from flask import Flask, jsonify, request
from flask_cors import CORS
import threading

from zyrex_state import state, ZyrexStatus, ModuleStatus
from zyrex_brain import brain
from zyrex_ai import ai

app = Flask(__name__)
CORS(app)


@app.route('/state', methods=['GET'])
def get_state():
    snap = state.snapshot()
    return jsonify({
        'status':          snap['status'].value,
        'current_command': snap['current_command'],
        'last_result':     snap['last_result'],
        'is_muted':        snap['is_muted'],
        'volume_level':    snap['volume_level'],
        'modules':         { n: s.value for n, s in snap['modules'].items() },
        'log_entries': [
            { 'timestamp': e.timestamp, 'level': e.level.value, 'message': e.message }
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


@app.route('/command', methods=['POST'])
def post_command():
    data = request.get_json()
    if not data or 'command' not in data:
        return jsonify({'error': 'No command'}), 400
    cmd = data['command'].strip()
    if not cmd:
        return jsonify({'error': 'Empty'}), 400
    threading.Thread(
        target=brain.process_text_command,
        args=(cmd,), daemon=True).start()
    return jsonify({'status': 'ok', 'command': cmd})


@app.route('/status', methods=['GET'])
def status():
    return jsonify({'status': 'running', 'version': '1.0'})


@app.route('/mic-test', methods=['GET'])
def mic_test():
    from zyrex_brain import test_microphone
    ok = test_microphone()
    return jsonify({'mic_ok': ok})


@app.route('/voice-toggle', methods=['POST'])
def voice_toggle():
    """Toggle voice listening on/off"""
    if brain._running:
        brain._running = False
        state.warning("Voice listening paused.")
        return jsonify({'voice': 'paused'})
    else:
        if brain.voice.is_loaded:
            brain._running = True
            import threading
            t = threading.Thread(target=brain._voice_loop, daemon=True)
            t.start()
            state.success("Voice listening resumed.")
            return jsonify({'voice': 'active'})
        else:
            return jsonify({'voice': 'whisper_not_loaded'}), 400


@app.route('/ai-status', methods=['GET'])
def ai_status():
    return jsonify(ai.ai_status())


@app.route('/speak', methods=['POST'])
def speak_text():
    data = request.get_json() or {}
    text = str(data.get("text", "")).strip()
    if not text:
        return jsonify({"error": "No text"}), 400
    threading.Thread(target=ai.speak, args=(text,), daemon=True).start()
    return jsonify({"status": "ok", "spoken": text})


@app.route('/ai-key', methods=['POST'])
def set_ai_key():
    data = request.get_json() or {}
    key = str(data.get("api_key", "")).strip()
    if not key:
        return jsonify({"error": "No api_key"}), 400
    ok = ai.set_api_key(key)
    if not ok:
        return jsonify({"status": "failed", "error": ai.ai_status().get("last_error", "")}), 400
    return jsonify({"status": "ok", "message": "Gemini API key updated."})


@app.route('/ai-command', methods=['POST'])
def ai_command():
    """
    Direct Gemini command endpoint:
    request: {"text":"your natural language task", "speak": true}
    """
    data = request.get_json() or {}
    text = str(data.get("text", "")).strip()
    speak = bool(data.get("speak", True))
    if not text:
        return jsonify({"error": "No text"}), 400

    commands = ai.process(text)
    response = ai.execute_and_respond(commands)
    if speak and response:
        threading.Thread(target=ai.speak, args=(response,), daemon=True).start()
    return jsonify({
        "status": "ok",
        "input": text,
        "commands": commands,
        "response": response,
    })


def start_brain_safe():
    try:
        brain.start()
    except Exception as e:
        state.error(f"Brain startup error: {e}")
        state.set_status(ZyrexStatus.IDLE)


if __name__ == '__main__':
    state.info("Starting Zyrex API...")
    threading.Thread(target=start_brain_safe, daemon=True).start()
    state.success("API running on http://localhost:5000")
    app.run(host='127.0.0.1', port=5000,
            threaded=True, debug=False, use_reloader=False)

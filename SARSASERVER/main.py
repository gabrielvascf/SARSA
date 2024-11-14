from flask import Flask, send_file, request, abort
import os

app = Flask(__name__)

# Path to the WAV file
WAV_FILE_PATH = "file.wav"

@app.route('/file.wav', methods=['GET'])
def serve_wav():
    # Check if the WAV file exists
    if not os.path.exists(WAV_FILE_PATH):
        abort(404, description="File not found")
    
    # Serve the file to the client
    return send_file(WAV_FILE_PATH, mimetype='audio/wav')

@app.route('/')
def index():
    return "ESP32 WAV Streaming Server"

if __name__ == '__main__':
    # Run the server on your machine's IP address and port 5000
    app.run(host='0.0.0.0', port=5000, debug=True)

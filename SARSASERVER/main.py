from flask import Flask, send_file, request, abort
import os

app = Flask(__name__)

# Path to the WAV file
WAV_FILE_PATH = "file.wav"

@app.route('/file.mp3', methods=['GET'])
def serve_mp3():
    # Check if the WAV file exists
    if not os.path.exists("file.mp3"):
        abort(404, description="File not found")

    return send_file("file.mp3", mimetype='audio/mp3')
@app.route("/<rfid>", methods=['GET'])
def get_file(rfid):
    print(rfid)
    # Check if the WAV file exists
    if not os.path.exists("./" + rfid):
        abort(404, description="File not found")
    
    return send_file("./" + rfid + "/file.mp3")

@app.route('/')
def index():
    return "ESP32 MP3 Streaming Server"

if __name__ == '__main__':
    # Run the server on your machine's IP address and port 5000
    app.run(host='0.0.0.0', port=5000, debug=True)

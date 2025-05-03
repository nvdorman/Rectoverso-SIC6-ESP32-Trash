from flask import Flask, jsonify

app = Flask(__name__)
status_servo = 0  # 0 = belum ada data, 1 = organik, 2 = anorganik

@app.route('/get-data', methods=['GET'])
def get_data():
    return jsonify({"status": status_servo})

@app.route('/set-status/<int:value>', methods=['GET'])
def set_status(value):
    global status_servo
    if value in [0, 1, 2]:
        status_servo = value
        return jsonify({
            "message": f"Status diubah menjadi {value}",
            "status": value
        })
    else:
        return jsonify({
            "error": "Nilai harus 0, 1, atau 2"
        }), 400

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=False)
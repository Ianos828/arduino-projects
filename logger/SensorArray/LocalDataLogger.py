from flask import Flask, request
import csv
import os
from datetime import datetime

app = Flask(__name__)

CSV_FILE = "measurements.csv"

#give Flask app all POST data
@app.route("/data", methods=["POST"])

#when data is received, this function processes the json format and appends it into a csv row entry
def receive_data():
    print("POST received!")
    data = request.json
    print(data)
    timestamp = datetime.now().isoformat()

    #format json into csv row
    row = [
        timestamp,
        data["fluorescence"],
	data["nir_680"],
        data["nir_705"],
        data["nir_730"],
        data["nir_760"],
        data["nir_810"],
        data["nir_860"],
        data["nir_940"],
        data["distance_mm"],
        data["ethylene_ppb"],
        data["air_quality"],
        data["mq3"],
        data["mq4"],
        data["mq5"]
    ]

    #writes the formatted row into csv file
    with open(CSV_FILE, "a", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(row)

    #returns OK status to ESP32
    return "OK", 200

#if file was run (not as an imported library)
if __name__ == "__main__":
    #if the file already exists, do not append a new header row
    if not os.path.exists(CSV_FILE):
        with open(CSV_FILE, "a", newline="") as f:
            writer = csv.writer(f)
            writer.writerow([
                "timestamp",
                "Fluorescence (mV)",
		"NIR (680nm)",
                "NIR (705nm)",
                "NIR (730nm)",
                "NIR (760nm)",
                "NIR (810nm)",
                "NIR (860nm)",
                "NIR (940nm)",
                "LiDAR Distance (mm)",
                "Ethylene (ppb)",
                "Air Quality Index (0-500)",
                "MQ3",
                "MQ4",
                "MQ5"
            ])
    app.run(host="0.0.0.0", port=5000)

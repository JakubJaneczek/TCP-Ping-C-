import json
from openpyxl import Workbook
import os

def extract_rtt_data_to_excel(input_file, output_excel):
    # Open and load the JSON file
    with open(input_file, 'r') as file:
        data = json.load(file)
    
    # Prepare a list to hold the RTT information
    rtt_data = []

    # Check and process the "intervals" section
    if "intervals" in data:
        for interval in data["intervals"]:
            for stream in interval.get("streams", []):
                rtt = stream.get("rtt")
                rttvar = stream.get("rttvar")
                snd_cwnd = stream.get("snd_cwnd")
                interval_start = interval.get("sum", {}).get("start")
                interval_end = interval.get("sum", {}).get("end")

                if rtt is not None:
                    rtt_data.append([
                        interval_start, interval_end, rtt, rttvar, snd_cwnd
                    ])

    # Check and process the "end" section
    if "end" in data and "streams" in data["end"]:
        for stream in data["end"]["streams"]:
            sender = stream.get("sender", {})
            if "mean_rtt" in sender:
                rtt_data.append([
                    0, data["end"].get("sum_sent", {}).get("end"), 
                    sender.get("mean_rtt"), None, sender.get("max_snd_cwnd")
                ])

    # Create a new Excel workbook
    workbook = Workbook()
    sheet = workbook.active
    sheet.title = "RTT Data"

    # Write the header row
    headers = ["Interval Start", "Interval End", "RTT", "RTTVar", "SndCwnd"]
    sheet.append(headers)

    # Write the RTT data to the Excel sheet
    for row in rtt_data:
        sheet.append(row)

    # Save the workbook
    workbook.save(output_excel)
    print(f"RTT data has been saved to {output_excel}")

def process_all_json_files(input_dir, output_dir):
    # Ensure the output directory exists
    os.makedirs(output_dir, exist_ok=True)

    # List all files in the input directory
    for filename in os.listdir(input_dir):
        # Check if the file has a .json extension
        if filename.endswith('.json'):
            input_file = os.path.join(input_dir, filename)
            output_file = os.path.join(output_dir, os.path.splitext(filename)[0] + '.xlsx')  # Change extension to .xlsx
            extract_rtt_data_to_excel(input_file, output_file)

# Run the script for all .json files in ./json directory and save to ./xlsx
if __name__ == "__main__":
    input_directory = "./json"
    output_directory = "./xlsx"
    process_all_json_files(input_directory, output_directory)

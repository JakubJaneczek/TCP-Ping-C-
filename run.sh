#!/bin/bash

# Define variables
SERVER_IP="176.16.2.1"     # Replace with the actual server IP
BASE_PORT=8000                # Replace with the actual port
OUTPUT_DIR="reorder10"         # Directory to save results
PROTOCOL="tcp"            # Protocol to use (tcp/udp)
EXECUTIONS=20             # Number of executions
CLIENT_EXEC="./client"    # Path to the compiled client binary

# Create output directory if it doesn't exist
mkdir -p $OUTPUT_DIR

# Loop through executions
for ((i = 1; i <= $EXECUTIONS; i++)); do
    PORT=$((BASE_PORT + i))
    OUTPUT_FILE="${OUTPUT_DIR}/run_${i}.csv"
    echo "Running execution $i..."
    $CLIENT_EXEC $PROTOCOL $SERVER_IP $PORT $OUTPUT_FILE
    echo "Results saved to $OUTPUT_FILE"
    sleep 5
done

echo "All executions completed. Results are in the '$OUTPUT_DIR' directory."


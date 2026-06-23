#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

PORT="${1:-/dev/ttyUSB0}"

# Check if secrets.h exists
if [ ! -f firmware/main/secrets.h ]; then
  echo "ERROR: firmware/main/secrets.h not found!"
  echo "Copy firmware/main/secrets.example.h and fill in your values."
  exit 1
fi

cd firmware
idf.py -p "$PORT" flash monitor

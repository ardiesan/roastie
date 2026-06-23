#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../firmware"
idf.py build
echo "Build complete. Flash with: scripts/flash.sh [port]"

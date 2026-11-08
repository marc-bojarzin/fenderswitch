#!/usr/bin/env bash
# Starts the fenderswitch web footboard.
#
# Usage: tools/footboard/start.sh [-p SERIAL_PORT] [-c MIDI_CHANNEL] [-H PORT] [-l]
#   -p  serial port of the Nano        (default: $SERIAL_PORT or /dev/ttyUSB0)
#   -c  default MIDI channel, 1-16     (default: $MIDI_CHANNEL or 1)
#   -H  HTTP port                      (default: $PORT or 8080)
#   -l  local only: listen on 127.0.0.1, not reachable from other devices
#
# Installs the dependencies on the first run (or after package-lock.json
# changed) and checks that no other program (serial monitor, USB MIDI bridge)
# holds the serial port. Stop with Ctrl+C.
set -euo pipefail

SCRIPT="$(readlink -f "$0")"
cd "$(dirname "$SCRIPT")"

usage() {
    sed -n '4,8p' "$SCRIPT" | sed 's/^# \{0,1\}//'
    exit "${1:-0}"
}

export SERIAL_PORT="${SERIAL_PORT:-/dev/ttyUSB0}"
export MIDI_CHANNEL="${MIDI_CHANNEL:-1}"
export PORT="${PORT:-8080}"
export HOST="${HOST:-0.0.0.0}"

while getopts "p:c:H:lh" opt; do
    case "$opt" in
        p) SERIAL_PORT="$OPTARG" ;;
        c) MIDI_CHANNEL="$OPTARG" ;;
        H) PORT="$OPTARG" ;;
        l) HOST=127.0.0.1 ;;
        h) usage 0 ;;
        *) usage 1 ;;
    esac
done

if ! command -v node >/dev/null || ! command -v npm >/dev/null; then
    echo "Node.js and npm are required: sudo apt install nodejs npm" >&2
    exit 1
fi

# Dependencies: install when missing or when the lock file is newer.
if [[ ! -d node_modules || package-lock.json -nt node_modules ]]; then
    echo "Installing dependencies ..."
    npm ci --no-audit --no-fund
    touch node_modules
fi

if [[ ! -e "$SERIAL_PORT" ]]; then
    echo "Note: $SERIAL_PORT not found yet. Plug in the Nano; the server keeps retrying." >&2
elif command -v fuser >/dev/null && fuser -s "$SERIAL_PORT" 2>/dev/null; then
    echo "$SERIAL_PORT is in use by another program:" >&2
    fuser -v "$SERIAL_PORT" >&2 || true
    echo "Close the serial monitor or the USB MIDI bridge first." >&2
    exit 1
fi

echo "Note: opening $SERIAL_PORT resets the Nano; the amp is in More Drive for about 2 s."
exec node server.js

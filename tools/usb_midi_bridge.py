#!/usr/bin/env python3
"""USB MIDI bridge for fenderswitch on Linux.

The Nano's USB chip (CH340) is a USB-serial converter, not a USB MIDI device.
This script opens a virtual ALSA MIDI port and forwards every message sent to
it over the serial port at the MIDI rate of 31250 baud. The firmware reads it
exactly like MIDI from the DIN socket.

It also prints the firmware's Serial output, so it replaces the serial monitor
while it runs. To keep using a serial monitor (e.g. pio device monitor) instead,
pass --no-monitor: the bridge then only sends. Otherwise both programs read the
port and each gets random parts of the output.

Requires: sudo apt install python3-mido python3-rtmidi python3-serial
Your user must be in the "dialout" group to open /dev/ttyUSB0.

Usage:
    python3 tools/usb_midi_bridge.py [--port /dev/ttyUSB0] [--name fenderswitch]
                                     [--no-monitor] [--quiet]

Then send to the MIDI port "fenderswitch" from a DAW or MIDI tool, or connect
a MIDI controller to it with aconnect / qpwgraph. Stop with Ctrl+C.
"""
import argparse
import sys
import threading
import time

import mido
import serial

MIDI_BAUD = 31250

# Opening the port resets the Nano (DTR auto-reset). Messages sent while the
# bootloader runs would be lost, so the bridge waits this long before it opens
# the MIDI port.
BOOT_WAIT_S = 2.5


def print_serial_output(ser, stop):
    """Prints the firmware's Serial output line by line until stop is set."""
    while not stop.is_set():
        try:
            line = ser.readline()
        except serial.SerialException as err:
            if not stop.is_set():
                print(f'serial: {err}', file=sys.stderr)
                stop.set()
            return
        if line:
            text = line.decode('ascii', errors='replace').rstrip()
            print(f'< {text}')


def main():
    parser = argparse.ArgumentParser(
        description='Forward MIDI from a virtual ALSA port to the '
                    'fenderswitch Nano over USB serial.')
    parser.add_argument('--port', default='/dev/ttyUSB0',
                        help='serial port of the Nano (default: %(default)s)')
    parser.add_argument('--name', default='fenderswitch',
                        help='name of the virtual MIDI port '
                             '(default: %(default)s)')
    parser.add_argument('--no-monitor', action='store_true',
                        help="don't read the firmware's output, leave it to a "
                             'separate serial monitor')
    parser.add_argument('--quiet', action='store_true',
                        help="don't print forwarded messages")
    args = parser.parse_args()

    try:
        ser = serial.Serial(args.port, MIDI_BAUD, timeout=0.2)
    except serial.SerialException as err:
        sys.exit(f'cannot open {args.port}: {err}')

    print(f'{args.port} open at {MIDI_BAUD} baud, '
          f'waiting {BOOT_WAIT_S} s for the Nano to boot ...')
    stop = threading.Event()
    if not args.no_monitor:
        reader = threading.Thread(target=print_serial_output,
                                  args=(ser, stop), daemon=True)
        reader.start()
    time.sleep(BOOT_WAIT_S)

    with mido.open_input(args.name, virtual=True,
                         client_name=args.name) as midi_in:
        print(f'MIDI port "{args.name}" ready. Ctrl+C to stop.')
        try:
            while not stop.is_set():
                # Poll instead of blocking, so Ctrl+C and a lost serial port
                # are noticed.
                for msg in midi_in.iter_pending():
                    ser.write(bytes(msg.bytes()))
                    if not args.quiet and msg.type != 'clock':
                        print(f'> {msg}')
                time.sleep(0.001)
        except KeyboardInterrupt:
            pass
        except serial.SerialException as err:
            print(f'serial: {err}', file=sys.stderr)
        finally:
            stop.set()
            ser.close()


if __name__ == '__main__':
    main()

# fenderswitch

A programmable, MIDI-controllable channel switcher for Fender™ Hot Rod Deluxe and Deville guitar amps.

## Overview

These amps ship with a simple footswitch for picking one of three channels: **Clean**, **Drive** and **More Drive**. The footswitch connects over a plain mono (TS) cable, and the amp selects the channel based on the voltage it sees on that cable.

fenderswitch replaces that footswitch. Two isolated optical CMOS relays (photorelays) recreate the analog switching of the original pedal:

- **Relay 1 (channel)** switches between **Clean** and **Drive**.
- **Relay 2 (gain)** chooses between **Drive** and **More Drive**.

On the microcontroller side, each relay is just a digital output pin driven HIGH or LOW.

### Front panel

There are three momentary push buttons, one for each channel, so you can select any channel directly. An LED next to each button shows which channel is active:

| Channel    | LED    |
|------------|--------|
| Clean      | green  |
| Drive      | yellow |
| More Drive | red    |

### Presets and MIDI

The microcontroller's EEPROM holds up to **128 presets**, one per MIDI program number. A MIDI IN circuit feeds the microcontroller's UART. When the device receives a **MIDI Program Change**, it loads the preset stored for that program and switches the amp to that channel. This lets you control the amp from an external MIDI pedal or a multi-effects board.

At boot, all presets are loaded from EEPROM into RAM, so switching programs never waits for the EEPROM. Preset *n* is stored in EEPROM byte *n* (bytes 0–127). Byte 128 holds the MIDI setting (0 = OMNI, 1–16 = channel), byte 129 the display brightness (0–7); the bytes above are free.

All programs start out as **Clean**, the factory preset. You change a preset with the channel buttons and store it by holding the button or by pressing **Store** (see [Usage](#usage)).

### Display and controls

An internal 4-digit 7-segment display (TM1637, with a colon) shows the current program number, and the settings menu for the MIDI channel (or OMNI mode) and the display brightness. A rotary encoder with a built-in push button lets you move through the menus and browse quickly through the presets. The **Store** button next to it saves a modified preset, and in the menu it confirms a setting and writes it to EEPROM.

## Features

- Recreates the amp's original 3-channel footswitch using isolated optical CMOS relays
- Three channel buttons with green, yellow and red status LEDs
- 128 presets stored in EEPROM, edited directly with the channel buttons
- MIDI IN: Program Change recalls the matching preset
- Configurable MIDI channel and OMNI mode
- 4-digit 7-segment display with menus, navigated by a rotary encoder with a push button
- Store button: saves a modified preset with one push, confirms settings in the menus
- Photorelays driven straight from the microcontroller's pins, no transistor stage

## How the amp's footswitch line works

The amp sends about 27 VAC through an 820 Ω resistor onto the tip of its FOOT SWITCH jack. Each half-cycle carries one piece of information:

- The **positive half-cycle** selects the **channel** (Clean or Drive).
- The **negative half-cycle** selects the **gain** (Drive or More Drive).

Whatever is plugged in clamps each half-cycle either to about one diode drop or leaves it open, in which case zener diodes inside the amp clamp it at about ±10 V. The amp measures the two peak voltages at test points TP32 and TP35 and decides with two comparators. In Clean mode the amp ignores the gain half-cycle, because transistor Q4 locks out More Drive while the Normal channel is active.

fenderswitch works like the amp's front-panel switches: a 1N4148 steering diode sends each half-cycle to its own photorelay, and the photorelay either shorts that half-cycle to the sleeve or leaves it open. This gives the largest noise margins of all switching methods.

![Footswitch line: D1 and relay 1 take the positive half-cycle, D2 and relay 2 the negative one, with a TVS across tip and sleeve](docs/footswitch_line.svg)

Source: Fender schematic 050384 rev. B and footswitch assembly 050419. The voltages below are derived from the schematic and should be measured on your own amp.

## Hardware

| Ref. | Component | Notes |
|------|-----------|-------|
| | Arduino Nano (ATmega328P) | CH340 clone with the old bootloader (uploads at 57600 baud), runs at 5 V |
| | 4-digit 7-segment display with colon, TM1637 module | CLK/DIO two-wire bus (not I2C) |
| | Rotary encoder with push button | Bare 5-pin mechanical encoder (EC11 style) |
| R × 2 | 10 kΩ | Encoder: pull-ups from A and B to 5 V |
| | 3 × LED | Green, yellow and red channel indicators, each with a series resistor |
| | 4 × momentary push button | One per channel, plus Store next to the encoder. Wired to GND, using the internal pull-ups, debounced in firmware |
| U1 | Toshiba TLP222A-2 (or 2 × TLP222A) | Dual photorelay, 60 V, 500 mA, R_ON ≤ 2 Ω. Ch. 1 = channel, ch. 2 = gain |
| R_F × 2 | 390 Ω, 1/4 W | In series between the GPIO and the relay LED's anode, about 9 mA at 5 V (use 200 Ω at 3.3 V) |
| D1, D2 | 1N4148 | Half-cycle steering. D1 anode to tip, D2 cathode to tip |
| TVS | SMBJ15CA (bidirectional) | ESD and plug-in transients across tip and sleeve |
| J | 5-pin DIN socket (180°) | MIDI IN |
| U | H11L1 or Sharp PC900V | MIDI optocoupler with Schmitt-trigger output, isolates the MIDI sender's ground. Same pinout: 1 anode, 2 cathode, 4 V_O, 5 GND, 6 V_CC |
| R × 2 | 220 Ω, 10 kΩ | MIDI: 220 Ω in series with the opto LED, 10 kΩ output pull-up to 5 V (keep it weak, see [Sharing the UART](#sharing-the-uart-with-usb)) |
| D | 1N4148 | MIDI: across the opto LED, against reverse voltage |
| C | 100 nF | MIDI: optocoupler decoupling, close to pin 6 |
| | Mono (TS) jack (6.35 mm) | Goes to the amp's footswitch input |
| | <!-- power supply --> | |

The KiCad 9 project is in [hardware/fenderswitch/](hardware/fenderswitch/). It was bootstrapped from [hardware/fenderswitch.py](hardware/fenderswitch.py) with [circuit-synth](https://github.com/circuit-synth/circuit-synth), which places the parts and connects them by net labels; the wiring is then done by hand in KiCad, and the `.kicad_sch` file is the source of truth. The channel buttons, the Store button and the power supply are not in it yet.

> **Availability:** Toshiba has announced end-of-life for the TLP222A. Buy spares, or use a part with a similar spec (60 V, ≥ 100 mA, R_ON ≤ 2–3 Ω), for example the Panasonic AQY212EH. Check the pinout and LED current before swapping.

Requirements for any replacement switch element: ≥ 60 V off-state voltage, ≥ 100 mA on-state current (the line carries about 46 mA peak), on-resistance ≤ 2 Ω (the Clean state has the smallest noise margin) and current flow in both directions. Phototransistor optocouplers, CMOS analog switches like the 4066 and plain low-side MOSFETs do not meet these.

### Relay inputs

The microcontroller drives each photorelay's input LED directly from its pin, through R_F to GND:

![Relay input: GPIO through R_F 390 Ω to the anode of the TLP222A LED, cathode to GND](docs/relay_driver.svg)

Pin HIGH: about (4.8 V − 1.15 V) / 390 Ω ≈ 9 mA flow through the LED and the relay closes. That's well above the TLP222A's maximum trigger current of 3 mA and well below the pin's 20 mA rating. Pin LOW: the LED sees 0 V and the relay is open.

**No fail-safe during reset.** While the microcontroller is in reset or running the bootloader (about 1–2 s on the old Nano bootloader), its pins are Hi-Z, so no current flows and both relays are open. The amp sees **More Drive**, its loudest setting, on every power-up and reset until the firmware starts and drives the pins HIGH (Clean). Turn the amp's volume down, or switch it to standby, while you power up, reset or upload to the Nano.

Keep the microcontroller side (5 V, GND, relay LEDs) and the amp side (tip, sleeve, D1, D2, TVS, relay outputs) apart on the board. With a separate power supply the photorelays keep the two grounds isolated, which avoids ground loops and hum.

### MIDI IN

The MIDI IN circuit follows the MIDI 1.0 electrical specification (2014):

![MIDI IN: DIN pin 4 through 220 Ω to the anode of the H11L1 / PC900V LED, cathode to DIN pin 5, 1N4148 across the LED. The open-collector output has a 10 kΩ pull-up to 5 V and goes to D0/RX; VCC is decoupled with 100 nF](docs/midi_in.svg)

- The sender drives a current loop through its own resistors, the 220 Ω and the optocoupler LED, about 5 mA. The 1N4148 protects the LED against reverse voltage.
- DIN pin 2 (shield) stays unconnected on the receiver, so the sender's ground never reaches ours.
- The optocoupler's Schmitt-trigger output goes LOW while the LED is lit, which matches the UART polarity, so no inverter is needed.
- Keep the pull-up at 10 kΩ, so the USB chip can still drive D0 for uploads (see [Sharing the UART with USB](#sharing-the-uart-with-usb)).

### Rotary encoder

A bare mechanical encoder with push button connects to the Nano with two 10 kΩ pull-ups. It has five pins: **A**, **C** and **B** on one side (C, the common contact, in the middle) and the two **SW** pins on the other.

![Rotary encoder: A to D2 and B to D4 (pin-change interrupt), each with a 10 kΩ pull-up to 5V; one SW pin to D5 (INPUT_PULLUP); C and the other SW pin to GND](docs/encoder.svg)

- The encoder only shorts A and B to C, and the push button shorts its two SW pins. So C and one SW pin go to GND, and A, B and SW need pull-ups.
- A and B get **10 kΩ to 5V**. That's stiffer than the Nano's internal pull-ups (20–50 kΩ), so the lines pick up less noise and their edges are faster, and the contacts carry about 0.5 mA, enough to keep them clean. SW is only polled, so the internal pull-up is enough there.
- **A** (CLK in the code) and **B** (DT) must both be on D0-D7 (port D), because the firmware decodes them with the port's pin-change interrupt (PCINT2). SW is polled and can be on any digital pin.
- The firmware turns on the internal pull-ups on A and B as well. They sit in parallel with the 10 kΩ (about 8 kΩ in total) and keep the encoder working if a resistor is missing.
- If you still see stray steps, the encoder datasheets suggest an RC filter per signal: 10 kΩ in series to the pin and 10 nF from the pin to GND. The quadrature decoder already cancels bounce, so this is optional.
- If turning right goes to the previous program, swap the A and B wires.

How the firmware decodes the signals is explained in [docs/rotary_encoder.md](docs/rotary_encoder.md).

### Prototype: LEDs instead of relays

For prototyping, the relay stages (photorelay and amp side) are replaced by two plain LEDs, one on OUT_CH and one on OUT_GAIN. Each LED is wired from the pin through a 220–330 Ω resistor to GND, like the channel LEDs. A lit LED means "relay ON" (closed), so the LEDs show the [switching states](#channel-switching-states) directly:

| Channel    | OUT_CH LED | OUT_GAIN LED |
|------------|------------|--------------|
| Clean      | on         | on           |
| Drive      | off        | on           |
| More Drive | off        | off          |

The LEDs are wired just like the photorelay inputs, so they behave the same, including during reset (both off). One thing to keep in mind:

- The 5 ms make-before-break steps are too short to see. To check the order of a switching sequence, watch the pins with a logic analyzer, or build with a much longer delay, for example `build_flags = -DRELAY_SETTLE_MS=500` in [platformio.ini](platformio.ini).

### Wiring

| Nano pin | Connected to | Notes |
|----------|--------------|-------|
| D0 (RX) | MIDI IN: optocoupler output (pin 4) | UART, 31250 baud. Shared with the USB chip, see below |
| D2 | Encoder A (CLK) | Pin-change interrupt (PCINT18), A and B must be on D0-D7 |
| D4 | Encoder B (DT) | Pin-change interrupt (PCINT20) |
| D5 | Encoder SW | "Enter" button |
| D7 | OUT_CH: R_F to relay 1 LED (channel) | HIGH = relay ON |
| D8 | OUT_GAIN: R_F to relay 2 LED (gain) | HIGH = relay ON |
| D9 | Red LED | More Drive, PWM (swell) |
| D10 | Yellow LED | Drive, PWM (swell) |
| D11 | Green LED | Clean, PWM (swell) |
| A4 | Display DIO | TM1637, bit-banged |
| A5 | Display CLK | TM1637, bit-banged |
| A0 | Clean button | To GND, `INPUT_PULLUP` |
| A1 | Drive button | To GND, `INPUT_PULLUP` |
| A2 | More Drive button | To GND, `INPUT_PULLUP` |
| A3 | Store button | To GND, `INPUT_PULLUP` |
| 5V / GND | Display VCC, encoder pull-ups, and GND for all modules (including encoder C and SW and the relay LED cathodes) | |

The channel LEDs must be on PWM pins, because a dirty preset makes its LED swell with `analogWrite()`. OUT_CH and OUT_GAIN only switch, so they use the non-PWM pins D7 and D8. Don't use D13 for OUT_CH or OUT_GAIN: the bootloader blinks the on-board LED on it, which would switch the relay.

### Checking the relay stages

If the amp doesn't react to a channel change, follow the signal from the Nano out to the relay with a multimeter (against GND unless noted):

1. **Serial monitor** (31250 baud): every change prints `program=… channel=…`. No line means the button or encoder isn't registering, and the relay stage isn't involved.
2. **D7 (OUT_CH) and D8 (OUT_GAIN):** about 4.8 V = ON, 0 V = OFF, as in [Channel switching states](#channel-switching-states). Clean = both HIGH, Drive = D7 LOW and D8 HIGH, More Drive = both LOW.
3. **Across R_F:** about 3.6 V when ON (about 9 mA through the relay LED), 0 V when OFF. If the pin is HIGH but there's no voltage across R_F, the LED is open or reversed: check its polarity (TLP222A-2: pin 1 anode, pin 2 cathode for channel 1; pin 3 anode, pin 4 cathode for channel 2). If the pin is pulled well below 4.8 V, the LED is shorted or R_F is too small.
4. **Relay output**, in ohms mode, with the amp side disconnected (TLP222A-2: pins 7–8 for channel 1, pins 5–6 for channel 2): a few Ω when ON, open when OFF. The photorelay has no visible indicator.

To slow the sequence down enough to follow on a meter or LEDs, build with `-DRELAY_SETTLE_MS=500` (see [Prototype: LEDs instead of relays](#prototype-leds-instead-of-relays)).

### Sharing the UART with USB

On the Nano, the USB chip (CH340) is connected to D0 (RX) and D1 (TX) through 1 kΩ resistors. MIDI IN only uses RX, so:

- **Serial output keeps working.** `Serial.print()` goes out on TX to the USB chip as before. Both directions share one baud rate, so the serial monitor runs at 31250 baud (`monitor_speed = 31250`).
- **Uploading works with MIDI IN connected.** The optocoupler's open-collector output and the USB chip's TX form a wired-AND on RX: either one can pull the line LOW. This only works because the opto's pull-up is weak (10 kΩ): the USB chip pulls the line to about 0.45 V through its 1 kΩ. With a strong pull-up, for example 470 Ω, the line stays at about 3.4 V and uploads fail.
- **No MIDI data while uploading.** Both sources share one line, so incoming MIDI would corrupt the upload. Stop the MIDI sender or unplug the MIDI cable.

### Channel switching states

"ON" means the photorelay is closed.

| Channel    | Relay 1 (OUT_CH) | Relay 2 (OUT_GAIN) | Expected TP32 | Expected TP35       |
|------------|------------------|--------------------|---------------|---------------------|
| Clean      | ON               | ON (don't care)    | ≈ +0.5 V      | ≈ −0.5 V (ignored)  |
| Drive      | OFF              | ON                 | ≈ +9.7 V      | ≈ −0.5 V            |
| More Drive | OFF              | OFF                | ≈ +9.7 V      | ≈ −9.9 V            |

TP32 and TP35 are test points inside the amp, where it measures the positive and negative peaks of the footswitch line. The amp's thresholds are about +1.1 to +1.4 V for the channel and about −4.1 to −4.9 V for the gain (both with hysteresis).

### Switching sequence and timing

The amp's peak detectors charge fast (820 Ω × 0.22 µF ≈ 0.2 ms) but discharge slowly (about 0.1 s). If a relay is open by mistake for even 1 ms and that gap falls on a mains peak, the amp jumps to the wrong mode for 50–100 ms, which you hear as a blip. So the firmware must never create an unintended "open" state and switches **make-before-break**. The TLP222A needs up to 2 ms to turn on, so wait **at least 5 ms** between the two steps:

| Target     | OUT_CH | OUT_GAIN | Order |
|------------|--------|----------|-------|
| Clean      | ON     | ON       | 1. OUT_CH = ON, 2. wait 5 ms, 3. OUT_GAIN = ON |
| Drive      | OFF    | ON       | 1. OUT_GAIN = ON, 2. wait 5 ms, 3. OUT_CH = OFF |
| More Drive | OFF    | OFF      | 1. OUT_GAIN = OFF (still locked out if coming from Clean), 2. wait 5 ms, 3. OUT_CH = OFF |

Rule of thumb: when leaving Clean, set the gain first and then the channel. When entering Clean, close the channel relay first.

```mermaid
stateDiagram-v2
    direction LR
    [*] --> MoreDrive: power-up / reset (pins Hi-Z, both relays open)
    Clean --> Drive: GAIN on, 5 ms, CH off
    Clean --> MoreDrive: GAIN off, 5 ms, CH off
    Drive --> Clean: CH on, 5 ms, GAIN on
    Drive --> MoreDrive: GAIN off
    MoreDrive --> Clean: CH on, 5 ms, GAIN on
    MoreDrive --> Drive: GAIN on
    MoreDrive: More Drive
```

Between Drive and More Drive only the gain relay changes, because the channel relay stays OFF in both.

A full channel change takes 20–100 ms on the amp side anyway, because of the slow discharge. The firmware doesn't need to be faster than that.

Firmware notes:

- Write HIGH to OUT_CH and OUT_GAIN **before** setting them to `OUTPUT`, so the pins go straight from Hi-Z to HIGH and the amp leaves More Drive for Clean as early as possible. The `OpticalRelay` constructor does this, before `setup()` runs.
- Start with the preset of program 1 (Clean on a fresh EEPROM).
- Enable the watchdog, so a hung microcontroller restarts. It passes through More Drive while it resets and boots.
- Debounce the buttons in software (about 30 ms).

## Firmware flow

```mermaid
flowchart TD
    MIDI["MIDI IN (UART, 31250 baud)"] --> PC{"Program Change?"}
    PC -- no --> Ignore["ignore"]
    PC -- yes --> CH{"MIDI channel matches<br>or OMNI on?"}
    CH -- no --> Ignore
    Boot["power-up:<br>load all presets<br>from EEPROM into RAM"] --> Load
    CH -- yes --> Load["drop dirty state (if any),<br>load preset for program number<br>into the working buffer"]
    Enc["rotary encoder<br>(browse presets)"] --> Load
    Load --> Mode["target channel"]
    Mode --> Apply["apply relay sequence<br>(make-before-break, 5 ms)"]
    Apply --> UI["update LEDs and display"]

    Btn["channel button pressed<br>(debounced)"] --> Diff{"differs from<br>stored preset?"}
    Diff -- "yes: dirty" --> Blink["swell channel LED"]
    Diff -- "no: clean" --> Steady["channel LED steady"]
    Blink --> Mode
    Steady --> Mode
    Hold["same button held 2 s<br>or Store pressed,<br>while dirty"] --> Store["store channel in preset<br>(EEPROM)"]
    Store --> Steady
```

## Testing

Bench test before connecting to the amp:

1. Measure the voltage across R_F and divide by R_F to get the LED current. Expect about 9 mA when ON and 0 when OFF.
2. With the amp unplugged, measure the relay outputs with an ohmmeter. Expect about 1–2 Ω when ON and open when OFF. Mind the D1 / D2 orientation if you measure from tip to sleeve.
3. Hold the Nano in reset with its reset button. Both relays read OFF (More Drive). After you release it and the bootloader has finished, both relays must switch ON (Clean).

Then on the amp, at low volume:

1. Measure TP32 and TP35 in every mode and compare them with the [switching states](#channel-switching-states). Clean must give TP32 clearly below 1.0 V.
2. Check the amp's front-panel LED: off for Clean, yellow for Drive, red for More Drive.
3. Step through all transitions repeatedly and listen for blips.
4. Unplug the switcher's power while the amp is on, and check which mode results.

> **Safety:** TP32 and TP35 are inside the amp, next to B+ voltages above 400 V, and the filter capacitors stay charged after switch-off. Discharge and measure before touching anything, use insulated probes, and keep one hand away from the chassis.

## Building and flashing

The project uses [PlatformIO](https://platformio.org/).

```sh
pio run                 # build
pio run -t upload       # flash to /dev/ttyUSB0
pio device monitor      # serial monitor at 31250 baud (the MIDI rate)
```

Adjust `upload_port` / `monitor_port` in [platformio.ini](platformio.ini) if your board shows up elsewhere.

The schematics in [docs/](docs/) are generated with [schemdraw](https://schemdraw.readthedocs.io/). After changing [docs/schematics.py](docs/schematics.py), re-render them:

```sh
pip install schemdraw
python docs/schematics.py
```

## Usage

### Selecting a program

Send a MIDI Program Change, or browse the presets with the rotary encoder: turn right for the next program, left for the previous one. Browsing wraps around, so turning right from program 128 gets you to program 1 and vice versa. The device loads the preset for that program and switches the amp to the stored channel. The display shows the current program (1–128). While you turn the encoder only the display follows; the amp switches once the encoder has rested for 300 ms (`BROWSE_SETTLE_MS`), so it doesn't jump through every channel on the way. A MIDI Program Change switches the amp right away.

### MIDI

Program Change 0–127 on the wire selects programs 1–128, as most MIDI gear numbers them. All other MIDI messages are ignored. The device listens on one MIDI channel or, in OMNI mode, on all of them; set this in the [settings menu](#settings-menu). The setting is kept in EEPROM. On a fresh EEPROM these build flags give the factory default:

| Build flag | Default | Meaning |
|------------|---------|---------|
| `MIDI_OMNI` | `1` | `1` = react to Program Change on every channel, `0` = only on `MIDI_CHANNEL` |
| `MIDI_CHANNEL` | `1` | MIDI channel 1–16, used while OMNI is off |

At startup the serial monitor shows the active setting (`midi: omni` or `midi: channel=5`), and each accepted message as `midi: program change ch=… pc=…`.

MIDI comes in on D0/RX, which the DIN socket's optocoupler and the Nano's USB chip share (see [Sharing the UART](#sharing-the-uart-with-usb)). So the same firmware handles both sources:

- **DIN cable** from a MIDI pedal or multi-effects board.
- **USB from the computer.** The CH340 is a USB-serial chip, not a USB MIDI device, so a bridge program on the computer has to forward MIDI to `/dev/ttyUSB0` at 31250 baud. Many tools (ttymidi, `stty`) reject 31250 because it's not a standard Linux rate, even though the CH340 supports it. Use [tools/usb_midi_bridge.py](tools/usb_midi_bridge.py) instead (see below).

#### USB MIDI bridge (Linux)

```sh
sudo apt install python3-mido python3-rtmidi python3-serial
python3 tools/usb_midi_bridge.py            # --port /dev/ttyUSB0 --name fenderswitch
```

The script opens a virtual ALSA MIDI port called **fenderswitch**. Send to it from a DAW or MIDI tool, or connect a controller to it with `aconnect` or qpwgraph. Lines starting with `>` are forwarded messages, lines with `<` are the firmware's serial output, so the bridge replaces the serial monitor while it runs. To use `pio device monitor` alongside it, start the bridge with `--no-monitor`: it then only sends, and the monitor gets the whole output. Without that option, both programs read the port and each gets random parts of the lines.

Opening the port resets the Nano (auto-reset over DTR), so the amp briefly goes to More Drive while the bootloader runs. The bridge waits 2.5 s before it opens the MIDI port, so no messages get lost in that time. Your user needs to be in the `dialout` group.

#### Web footboard

[tools/footboard/](tools/footboard/) is a small Node.js test rig with two separate web devices, connected only through the Nano's USB serial port:

- **MIDI footboard** (`/footboard.html`): behaves like a plain hardware footboard. Four footswitches per bank (32 banks for programs 1–128), bank up/down, direct entry, a MIDI channel setting (1–16, remembered in the browser), and a 3-digit 7-segment display with the program last sent. It only sends Program Change and knows nothing about the presets. It writes the bytes straight to the serial port, so it needs neither the bridge nor ALSA.
- **Device monitor** (`/monitor.html`): the fenderswitch's 7-segment display (program number, or the menu entry while the menu is open, at the device's brightness) and channel LEDs, plus a log of the MIDI bytes sent and the firmware's serial output. It reads the firmware's debug text, a return path that only exists because USB carries both directions; a real MIDI cable has none.

The start page (`/`) shows both side by side. Each one also works on its own, for example the footboard on a phone.

```sh
sudo apt install nodejs npm
tools/footboard/start.sh        # http://localhost:8080
tools/footboard/start.sh -h     # options: -p serial port, -c default MIDI channel, -H HTTP port, -l local only
```

The script installs the dependencies on the first run and refuses to start while another program (serial monitor, bridge) holds the serial port. The options override these environment variables:

| Variable | Default | Meaning |
|----------|---------|---------|
| `SERIAL_PORT` | `/dev/ttyUSB0` | Serial port of the Nano |
| `MIDI_CHANNEL` | `1` | Default MIDI channel to send on, 1–16, until the channel is changed on the footboard page |
| `PORT` | `8080` | HTTP port |
| `HOST` | `0.0.0.0` | Listen address. The default makes the page reachable from a phone or tablet on the same network, without a password. Use `127.0.0.1` to keep it local |

Keys on the page: 1–4 for the footswitches, ← → for the bank. Like the bridge, the server resets the Nano when it opens the port and reads the port itself, so don't run it together with the bridge or a serial monitor; its page has a log of the firmware's output instead.

Don't send from both at the same time: the two streams would mix on the shared line.

### Editing a preset

All 128 presets are initialized to **Clean** in the EEPROM. This is the **factory preset**.

1. Press one of the three channel buttons, for example **Drive**. The amp switches to that channel right away.
2. If the new channel differs from the one stored in the preset, its LED **swells** (fades up and down). This means the preset is **dirty**: the current state is modified but not saved.
3. Press the button of the stored channel to go back to it. The preset is clean again and the LED lights steadily.
4. To keep the change instead, **hold the same channel button for 2 seconds**, or **press Store** once. The new channel is written to the preset in EEPROM and the LED stops swelling. If the preset has no unsaved change, there is nothing to store: the LED goes dark for a moment to show that the press was registered.

A program change while the preset is dirty, whether by MIDI Program Change or with the encoder, drops the modified state without saving it. The preset keeps its stored channel.

### Settings menu

Press the encoder's push button to open the settings menu. Turning the encoder browses the entries of the menu you're in, wrapping around at both ends; the push button or **Store** picks the entry shown.

| Top menu | Opens |
|----------|-------|
| `Chan` | The MIDI channel submenu, on the current setting |
| `dISP` | The display brightness submenu, on the current level |
| `ESC` | Leaves the menu |

| MIDI submenu | Meaning |
|--------------|---------|
| `ALL` | OMNI: react to Program Change on every channel (a 7-segment display can't draw the "m" of "Omni") |
| `CH:01` … `CH:16` | React only to this MIDI channel |
| `ESC` | Back to the top menu without saving |

| Display submenu | Meaning |
|-----------------|---------|
| `br:01` … `br:08` | Brightness of the 7-segment display, dimmest to brightest. The display takes each level right away while you turn, so you see what you get |
| `ESC` | Back to the top menu; the display goes back to the stored brightness |

Picking a value in a submenu writes it to EEPROM, takes effect right away and leaves the menu; the display shows the program number again. While the menu is open, turning the encoder doesn't change the program, but MIDI and the channel buttons keep working. On a fresh EEPROM the brightness starts at `br:05` (`SEGMENT_BRIGHTNESS`, 0–7, in [SegmentDisplay.hpp](include/SegmentDisplay.hpp)).

## Code structure

| File | Purpose |
|------|---------|
| [src/main.cpp](src/main.cpp) | Setup and main loop |
| [include/Task.hpp](include/Task.hpp) | Cooperative tasks (protothread style) and the round-robin scheduler, no `delay()` anywhere |
| [include/RotaryEncoder.hpp](include/RotaryEncoder.hpp) | Decodes the rotary encoder (quadrature, pin-change interrupt on both pins) and debounces its push button, explained in detail in [docs/rotary_encoder.md](docs/rotary_encoder.md) |
| [include/SegmentDisplay.hpp](include/SegmentDisplay.hpp) | 4-digit TM1637 7-segment display: numbers, short texts, colon; talks to the chip only when something changed |
| [include/LcdBuffer.hpp](include/LcdBuffer.hpp) | Shadow buffer for the 16x2 LCD, writes only the characters that changed (currently unused, replaced by the 7-segment display) |
| [include/AmpChannel.hpp](include/AmpChannel.hpp) | The three amp channels (Clean, Drive, More Drive) |
| [include/Preset.hpp](include/Preset.hpp) | The channel stored for one MIDI program, loads from and flushes to the EEPROM byte of a given program number |
| [include/PresetLoader.hpp](include/PresetLoader.hpp) | All 128 presets in RAM plus the working buffer, dirty tracking and storing |
| [include/OpticalRelay.hpp](include/OpticalRelay.hpp) | Drives the two photorelays, switching make-before-break |
| [include/Led.hpp](include/Led.hpp) | On/off LED with slow/fast blinking, swelling (PWM), a one-shot flash and a short wink (off, then on again) |
| [include/EncoderTask.hpp](include/EncoderTask.hpp) | Browses the presets with the encoder (right = next program, wraps around), "Enter" opens the settings menu (MIDI channel, display brightness; Enter or Store confirms), Store saves a dirty preset. Also shows the working buffer: steady or swelling LED while dirty |
| [include/Settings.hpp](include/Settings.hpp) | The menu settings: MIDI (OMNI or channel 1–16) and display brightness, in EEPROM bytes 128 and 129 |
| [include/MidiTask.hpp](include/MidiTask.hpp) | MIDI IN: reads the UART, filters by the MIDI channel or OMNI from Settings, and recalls the preset of each Program Change right away |
| [include/MidiParser.hpp](include/MidiParser.hpp) | Byte-by-byte MIDI parser: running status, real-time bytes in the middle of messages, SysEx and stray data bytes |
| [include/ChannelTask.hpp](include/ChannelTask.hpp) | The three channel buttons: a press switches the amp right away, holding the current channel's button for 2 s stores a dirty preset |
| [include/Button.hpp](include/Button.hpp) | Debounced push button to GND, with hold detection (channel buttons, Store, the encoder's "Enter") |
| [tools/footboard/](tools/footboard/) | Test rig (Node.js): a web MIDI footboard that sends Program Change over USB serial, and a separate device monitor that shows the firmware's display and serial output |
| [tools/usb_midi_bridge.py](tools/usb_midi_bridge.py) | Linux: virtual ALSA MIDI port, forwarded over USB serial at 31250 baud, prints the firmware's serial output |
| [include/StatsTask.hpp](include/StatsTask.hpp) | Prints loop passes per second over Serial, to spot blocking code. Off by default, turn it on with `-DSTATS_INTERVAL_S=10` in `build_flags` (development only) |

## License

<!-- e.g. MIT -->

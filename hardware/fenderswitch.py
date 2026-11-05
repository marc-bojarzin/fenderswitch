"""Bootstraps the KiCad project in hardware/fenderswitch/ with circuit-synth.

Run once from the repository root: python hardware/fenderswitch.py
(needs pip install circuit-synth and KiCad 9 with its symbol libraries).

circuit-synth only places the parts and connects them by net labels. The
schematic is then laid out and wired by hand in KiCad, and the .kicad_sch file
is the source of truth from there on. Running this script again regenerates
the schematic and loses that manual work.

circuit-synth writes hierarchical labels even on a single sheet; they are
turned into local labels afterwards. It also leaves the Nano's second GND pin
(29) unconnected, so wire that one by hand.

Not included yet: the three channel buttons, the Store button and the power
supply (see the open items in the README).
"""
import re
from pathlib import Path

from circuit_synth import Component, Net, circuit

OUT = Path(__file__).parent / 'fenderswitch'


def resistor(value):
    return Component(symbol='Device:R', ref='R', value=value)


@circuit(name='fenderswitch')
def fenderswitch():
    vcc = Net('+5V')
    gnd = Net('GND')
    # Amp side, isolated from the microcontroller side by the photorelays.
    tip = Net('TIP')
    sleeve = Net('SLEEVE')

    nano = Component(symbol='MCU_Module:Arduino_Nano_v3.x', ref='A',
                     value='Arduino Nano')
    nano['+5V'] += vcc
    nano[4] += gnd
    nano[29] += gnd

    # Amp side first, so the references match the README (D1, D2, Q1, Q2).

    # Relay 1 (channel) on D9 with D1 on the positive half-cycle, relay 2
    # (gain) on D10 with D2 on the negative one. Each relay LED is driven by an
    # NPN low-side switch; R_P keeps it ON while the pin is Hi-Z.
    for pin, name in (('D9', 'OUT_CH'), ('D10', 'OUT_GAIN')):
        out = Net(name)
        base = Net(f'{name}_B')
        led_a = Net(f'{name}_LED_A')
        led_k = Net(f'{name}_LED_K')
        steer = Net(f'{name}_STEER')

        nano[pin] += out
        r_b = resistor('1k')
        r_p = resistor('10k')
        r_f = resistor('390')
        q = Component(symbol='Transistor_BJT:BC547', ref='Q', value='BC547')
        relay = Component(symbol='Relay_SolidState:TLP222A', ref='U',
                          value='TLP222A')
        diode = Component(symbol='Diode:1N4148', ref='D', value='1N4148')

        r_b[1] += out
        r_b[2] += base
        r_p[1] += vcc
        r_p[2] += base
        q['B'] += base
        q['E'] += gnd
        q['C'] += led_k
        r_f[1] += vcc
        r_f[2] += led_a
        relay[1] += led_a
        relay[2] += led_k

        # D1: anode to tip. D2: cathode to tip.
        if name == 'OUT_CH':
            diode['A'] += tip
            diode['K'] += steer
        else:
            diode['K'] += tip
            diode['A'] += steer
        relay[3] += steer
        relay[4] += sleeve

    tvs = Component(symbol='Device:D_TVS', ref='D', value='SMBJ15CA')
    tvs['A1'] += tip
    tvs['A2'] += sleeve

    jack = Component(symbol='Connector_Audio:AudioJack2', ref='J',
                     value='FOOT SWITCH')
    jack['T'] += tip
    jack['S'] += sleeve

    # Channel LEDs: pin -> series resistor -> LED -> GND.
    for pin, color in (('D6', 'green'), ('D7', 'yellow'), ('D8', 'red')):
        out = Net(f'LED_{color.upper()}')
        led_a = Net(f'LED_{color.upper()}_A')
        r = resistor('330')
        led = Component(symbol='Device:LED', ref='D', value=color)
        nano[pin] += out
        r[1] += out
        r[2] += led_a
        led['A'] += led_a
        led['K'] += gnd

    # Rotary encoder module (KY-040): CLK, DT, SW, +, GND.
    enc = Component(symbol='Connector_Generic:Conn_01x05', ref='J',
                    value='KY-040')
    for pin, name, header_pin in (('D2', 'ENC_CLK', 1), ('D4', 'ENC_DT', 2),
                                  ('D5', 'ENC_SW', 3)):
        net = Net(name)
        nano[pin] += net
        enc[header_pin] += net
    enc[4] += vcc
    enc[5] += gnd

    # LCD I2C backpack: GND, VCC, SDA, SCL.
    lcd = Component(symbol='Connector_Generic:Conn_01x04', ref='J',
                    value='LCD I2C')
    lcd[1] += gnd
    lcd[2] += vcc
    for pin, name, header_pin in (('A4', 'SDA', 3), ('A5', 'SCL', 4)):
        net = Net(name)
        nano[pin] += net
        lcd[header_pin] += net

    # MIDI IN (MIDI 1.0 electrical spec, 2014): DIN pin 4 -> 220R -> opto LED
    # -> DIN pin 5, with a 1N4148 across the LED against reverse voltage. DIN
    # pin 2 (shield) stays unconnected on the receiver, so the sender's ground
    # never reaches ours. The opto is an H11L1 or a PC900V: same pinout, both
    # with a Schmitt-trigger output that goes LOW while the LED is lit. The
    # open-collector output goes straight to D0/RX, which the Nano's USB chip
    # also drives through 1k. The weak 10k pull-up lets either side pull the
    # line LOW (wired-AND), so uploading over USB works with MIDI IN connected,
    # as long as no MIDI data arrives during the upload.
    midi_src = Net('MIDI_SRC')
    midi_a = Net('MIDI_LED_A')
    midi_k = Net('MIDI_LED_K')
    midi_out = Net('MIDI_OUT')

    din = Component(symbol='Connector:DIN-5_180degree', ref='J',
                    value='MIDI IN')
    din[4] += midi_src
    din[5] += midi_k

    r_in = resistor('220')
    r_in[1] += midi_src
    r_in[2] += midi_a

    opto = Component(symbol='Isolator:H11L1', ref='U', value='H11L1')
    opto[1] += midi_a
    opto[2] += midi_k
    opto[4] += midi_out
    opto[5] += gnd
    opto[6] += vcc

    d_rev = Component(symbol='Diode:1N4148', ref='D', value='1N4148')
    d_rev['A'] += midi_k
    d_rev['K'] += midi_a

    # Pull-up for the output (weak, see above), decoupling.
    r_pu = resistor('10k')
    r_pu[1] += vcc
    r_pu[2] += midi_out
    c = Component(symbol='Device:C', ref='C', value='100n')
    c[1] += vcc
    c[2] += gnd
    nano[2] += midi_out  # D0/RX


def use_local_labels(sch):
    """Turns hierarchical labels into local ones (single-sheet design)."""
    text = sch.read_text()
    text = re.sub(r'\(hierarchical_label (".*?")\s*\(shape \w+\)',
                  r'(label \1', text)
    sch.write_text(text)


if __name__ == '__main__':
    fenderswitch().generate_kicad_project(
        str(OUT), generate_pcb=False, update_source_refs=False)
    use_local_labels(OUT / 'fenderswitch.kicad_sch')

"""Renders the README schematics to SVG with schemdraw (pip install schemdraw).

Run from the repository root: python docs/schematics.py
"""
from pathlib import Path

import schemdraw
import schemdraw.elements as elm

OUT = Path(__file__).parent
schemdraw.use('svg')


def footswitch_line():
    """Tip/sleeve side: steering diodes, photorelay outputs and TVS."""
    with schemdraw.Drawing(file=OUT / 'footswitch_line.svg', show=False) as d:
        d.config(unit=2.5, bgcolor='white')
        elm.Dot(open=True).label('TIP', 'left')
        elm.Line().right(1)
        elm.Dot()
        d.push()
        # Positive half-cycle -> relay 1 (channel).
        elm.Diode().right().label('D1\n1N4148')
        elm.Switch(action='close').right().label('Relay 1\n(channel)')
        elm.Line().right(1)
        elm.Dot()
        d.push()
        elm.Line().right(1)
        elm.Dot(open=True).label('SLEEVE\n(amp GND)', 'right')
        d.pop()
        bus_top = elm.Line().down(2.5)
        d.pop()
        elm.Line().down(2.5)
        elm.Dot()
        d.push()
        # Negative half-cycle -> relay 2 (gain).
        elm.Diode().right().reverse().label('D2\n1N4148', 'bottom')
        elm.Switch(action='close').right().label('Relay 2\n(gain)', 'bottom')
        elm.Line().right(1)
        elm.Dot()
        d.pop()
        elm.Line().down(2.5)
        elm.Zener().right().tox(bus_top.end).label('TVS SMBJ15CA', 'bottom')
        elm.Line().up(2.5)


def relay_driver():
    """A TLP222A input LED driven straight from the GPIO through R_F."""
    with schemdraw.Drawing(file=OUT / 'relay_driver.svg', show=False) as d:
        d.config(unit=2.5, bgcolor='white')
        elm.Dot(open=True).label('GPIO\n(D7 / D8)', 'left')
        elm.Resistor().right().label('R_F\n390Ω')
        elm.Line().right(0.5)
        elm.LED().down().label('TLP222A\nLED', 'bottom')
        elm.Ground()


def midi_in():
    """MIDI IN: DIN socket, H11L1 / PC900V optocoupler, output to D0/RX."""
    with schemdraw.Drawing(file=OUT / 'midi_in.svg', show=False,
                           margin=1.5) as d:
        d.config(unit=2.5, bgcolor='white')
        opto = elm.Ic(pins=[
            elm.IcPin(name='A', pin='1', side='left', slot='2/2'),
            elm.IcPin(name='K', pin='2', side='left', slot='1/2'),
            elm.IcPin(name='VCC', pin='6', side='right', slot='3/3'),
            elm.IcPin(name='VO', pin='4', side='right', slot='2/3'),
            elm.IcPin(name='GND', pin='5', side='right', slot='1/3'),
        ], edgepadW=1.5, pinspacing=1.5, leadlen=1).label(
            'U\nH11L1 /\nPC900V', 'center')

        # Input: DIN pin 4 -> 220R -> LED anode, LED cathode -> DIN pin 5,
        # 1N4148 across the LED.
        elm.Line().at(opto.A).left(1)
        a = elm.Dot()
        elm.Resistor().left().label('220Ω')
        elm.Dot(open=True).label('DIN pin 4', 'left')
        elm.Line().at(opto.K).left(1)
        k = elm.Dot()
        elm.Line().left().tox(a.start.x - 2.5 - 1)  # past the resistor
        elm.Dot(open=True).label('DIN pin 5', 'left')
        elm.Diode().at(k.center).toy(a.center).label('1N4148', 'left')
        elm.Label().at((a.center.x - 3.5, k.center.y - 1.5)).label(
            'DIN pin 2 (shield): not connected', 'right')

        # Supply with decoupling.
        elm.Line().at(opto.VCC).right(1)
        elm.Line().up(1)
        vcc = elm.Dot()
        elm.Vdd().label('+5V')
        elm.Line().at(vcc.center).right(1.5)
        elm.Capacitor().down(1.5).label('100n', 'bottom')
        elm.Ground()

        # Output: open collector, weak pull-up, straight to D0/RX.
        elm.Line().at(opto.VO).right(5)
        out = elm.Dot()
        elm.Line().right(1)
        elm.Dot(open=True).label('D0/RX\n(USB chip TX\nvia 1k on the Nano)',
                                 'right')
        elm.Resistor().at(out.center).up().label('10k', 'bottom')
        elm.Vdd().label('+5V')

        elm.Line().at(opto.GND).right(1)
        elm.Ground()


def encoder():
    """Bare 5-pin rotary encoder with push button (EC11 style) to the Nano.

    No pull-ups on the encoder: A, B and SW use the Nano's internal ones.
    """
    with schemdraw.Drawing(file=OUT / 'encoder.svg', show=False,
                           margin=1) as d:
        d.config(unit=2.5, bgcolor='white')
        # Encoder pins: A, C, B on one side, the two SW pins on the other.
        enc_pins = ['A', 'B', 'SW', 'C', 'SW']
        nano_pins = ['D2', 'D4', 'D5', 'GND']
        n = len(enc_pins)
        enc = elm.Ic(pins=[
            elm.IcPin(name=name, anchorname=f'e{i}', side='right',
                      slot=f'{n - i}/{n}')
            for i, name in enumerate(enc_pins)
        ], edgepadW=3, pinspacing=1.2, leadlen=0.5).label(
            'Rotary\nencoder\nwith push\nbutton', 'center')
        nano = elm.Ic(pins=[
            elm.IcPin(name=name, anchorname=f'n{i}', side='left',
                      slot=f'{n - 1 - i}/{n - 1}')
            for i, name in enumerate(nano_pins)
        ], edgepadW=1.8, pinspacing=1.2, leadlen=0.5).label(
            'Arduino\nNano', 'center').anchor('n0').at(
                (enc.e0[0] + 9, enc.e0[1]))

        # A, B and SW straight across, C to GND.
        notes = ['PCINT18', 'PCINT20', 'INPUT_PULLUP', '']
        for i, note in enumerate(notes):
            wire = elm.Line().at(getattr(enc, f'e{i}')).to(
                getattr(nano, f'n{i}'))
            if note:
                wire.label(note, 'top', fontsize=10, ofst=(1.5, 0.1))

        # 10k pull-ups to +5V on A and B, tops level. B's lead crosses the A
        # wire without connecting.
        x_a, y_a = enc.e0[0] + 1.5, enc.e0[1]
        x_b, y_b = enc.e0[0] + 3, enc.e1[1]
        elm.Dot().at((x_a, y_a))
        elm.Line().up(0.6)
        elm.Resistor().up(2).label('10k', 'top')
        elm.Vdd().label('+5V')
        elm.Dot().at((x_b, y_b))
        elm.Line().up(y_a - y_b + 0.6)
        elm.Resistor().up(2).label('10k', 'bottom')
        elm.Vdd().label('+5V')
        # Second SW pin joins the GND wire.
        elm.Line().at(enc.e4).right(1.5)
        elm.Line().up().toy(enc.e3)
        elm.Dot()


if __name__ == '__main__':
    footswitch_line()
    relay_driver()
    midi_in()
    encoder()

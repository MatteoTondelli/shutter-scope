# Shutter Scope
Arduino-based intervalometer with embedded telescope focuser, for astrophotography.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

I suggest you to use the ready-made board; otherwise, if you want to build it on a breadboard, this is a list of hardware you need:

- Arduino UNO board or similar.
- LCD Hitachi HD44780 (red color preferred).
- Rotary encoder with embedded pushbutton.
- 28BYJ-48 Stepper motor with ULN2003 driver board.
- Optocupler.
- 4x 10k$\Omega$ resistor.
- 1x 330$\Omega$ resistor.
- 1x 2N3904 transistor (any equivalent NPN is ok).
- Generic buzzer (optional).

### Installing

The easisest way to upload new code on the baord is to have an Arduino UNO board, in this way you'll simply need to place the ATmega328P-PU on it and flash your code.

## Running the tests

Just power up the board.

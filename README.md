# organ-piston-midi-control-board

This is the firmware that runs the boards that control my home pipe organ's thumb pistons and toe studs.

It talks to a bunch of MCP23017 I/O expanders that are connected to the individual pistons and toe studs and acts like
a USB MIDI adapter that triggers note on/off events whenever pistons are pressed or released.

In the future, it will also act as a Bluetooth keyboard and trigger key presses whenever certain pistons are pressed -
that will allow it to connect to sheet music apps like [Newzik](https://newzik.com/) and flip to the next/previous page
via a thumb piston or toe stud press.

# Development and installation

[TBD]

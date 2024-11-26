# Info
- examples for handling audio and midi data
- uses Jack Audio Connection Kit

# Idea
- have a bunch of good examples in low level C

# Examples
## Jack Midi in
- simple Midi message debugger
- shows incomming Midi messages in terminal
- transport midi data through ringbuffer

## Jack Sine Out
- create a Sinewave and play them on output
- create of signal data and transport through ringbuffer

# Build
## Dependencies
- `Jack` (maybe `jack2` or `pipewire-jack`)
- `cmake` && `make` as build system
## how to build
- run:
  - `mkdir build`
  - `cd build`
  - `cmake ..`
  - `make`

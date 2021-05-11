# gpsp for BB10

A port from the famous gpsp, to run on BB10 devices

Require GBA bios file (not part of the code release)
- Place GBA bios file "gba_bios.bin" in misc/roms/gba

Currently supporting features:
- Game state saving
- Game state loading
- GBA save file from VBAM can be used in gpsp
- Screen resize (Normal aspect ratio / fullscreen)
- GPU filter selection (0: No filter, 1: FXAA filter)

gpsp Configuration file
- configuration file located in misc/gbaemu/gpsp.cfg
- User can edit key mapping in gpsp.cfg
- User can select GPU filter type
- User can enable debug logging when reporting problems

Binary link
https://app.box.com/s/oylizwfv3cmgpk61aywjoqawo1b9x0rb

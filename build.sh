#!/bin/bash
set -e

windres config_window.rc config_window.o
gcc *.c config_window.o -std=c11 -Wall -Wextra -Wpedantic -Wno-unused-parameter -shared -static-libgcc -static -lshlwapi -lgdi32 -l:libusb-1.0.a -o pj64-wiiu-gcn.dll

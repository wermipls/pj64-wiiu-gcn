windres config_window.rc config_window.o
gcc *.c config_window.o -shared -static-libgcc -static -lshlwapi -l:libusb-1.0.a -o pj64-wiiu-gcn.dll

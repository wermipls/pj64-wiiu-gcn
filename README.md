# pj64-wiiu-gcn
Proof of concept Wii U Gamecube adapter plugin for Project64.
## Download
Latest release can be found [here](https://github.com/wermipls/pj64-wiiu-gcn/releases).

## Usage instructions
Copy the .dll to the Plugin directory, then select the plugin in the emulator.

If not done already, you will need to install the WinUSB driver with Zadig. If you have a third party adapter, you need to switch it to Wii U mode. See [this page](https://wiki.dolphin-emu.org/index.php?title=How_to_use_the_Official_GameCube_Controller_Adapter_for_Wii_U_in_Dolphin#Using_Zadig) for more detailed instructions.

## Building
Use MSYS2 MINGW32 with gcc and libusb installed.

As MSYS2 doesn't provide libusb for MINGW32 anymore, you'll have to build and install it yourself. Assuming you have all the necessary packages:
```sh
git clone https://github.com/libusb/libusb
cd libusb
./bootstrap.sh && ./configure && make -j && make install
```

Run `build.sh` to compile.

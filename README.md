# pj64-wiiu-gcn
Proof of concept Wii U Gamecube adapter plugin for Project64.
## Download
Latest release can be found [here](https://github.com/wermipls/pj64-wiiu-gcn/releases).

## Usage instructions
Copy the .dll to the Plugin directory, then select the plugin in the emulator.

If not done already, you will need to install the WinUSB driver with Zadig. If you have a third party adapter, you need to switch it to Wii U mode. See [this page](https://wiki.dolphin-emu.org/index.php?title=How_to_use_the_Official_GameCube_Controller_Adapter_for_Wii_U_in_Dolphin#Using_Zadig) for more detailed instructions.

## Building
Use MSYS2 with the following packages installed:
* `mingw-w64-i686-gcc`
* `mingw-w64-i686-libusb`

Run `build.sh` to compile.

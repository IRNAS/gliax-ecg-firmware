# gliax-ecg-firmware

Gliax ECG firmware

www.irnas.eu

## Compiling the code
### Online compiler
Import code from URL (https://github.com/IRNAS/gliax-ecg-firmware).
Select platform for which code will be compiled and hit "Compile" button.
Online compiler will offer you to save hex file.

### mbed-cli
In order to compile the code localy, it is required to install mbed-cli and all dependency tools.\
See: https://github.com/ARMmbed/mbed-cli \
Then it is required to import code from GitHub:\
`$ mbed import https://github.com/IRNAS/gliax-ecg-firmware`\
After import is finished, execute mbed compile command with desired target and tools selected, for example:\
`$ mbed mbed compile -m SEEED_TINY_BLE -t GCC_ARM`\
Resulting hex file will be located in folder ./BUILD/SEED_TINY_BLE/GCC_ARM/

## Download code to the board
In order to download code to mbed platform, just copy hex file to the MBED drive. 


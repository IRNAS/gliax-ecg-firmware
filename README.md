# gliax-ecg-firmware

Gliax ECG firmware

www.irnas.eu

## Compiling the code

### Online compiler

- Import code with URL (https://github.com/IRNAS/gliax-ecg-firmware).
- Select the platform for which code will be compiled and hit the **Compile** button.
- Online compiler will offer you to save the hex file.

### `mbed-cli`

To compile the code locally, install
[`mbed-cli`](https://github.com/ARMmbed/mbed-cli) and its dependencies.

Import code from GitHub:

```shell
$ mbed import https://github.com/IRNAS/gliax-ecg-firmware
```

Compile the code with the desired target and tools e.g. `SEEED_TINY_BLE`:

```shell
$ mbed mbed compile -m SEEED_TINY_BLE -t GCC_ARM
```

The resulting hex file will be located in folder `./BUILD/SEED_TINY_BLE/GCC_ARM/`.

## Download code to the board

To download code to the `mbed` platform, copy the hex file to the `MBED` drive.

## Configuring CP2102N chip

The chip for ECG measurements needs to be turned on by CP2102N. This
configuration can be done with [Simplicity
Studio](https://www.silabs.com/products/development-tools/software/simplicity-studio).

Procedure:

- Connect the board to PC with a USB cable
- Open Simplicity Studio and create a new project (configure chip CP2102N
  revision: A02-new or A01-old chip)
- Open Port configuration: Serial
- Change pin GPIO5's both modes to Push-Pull and reset Latch to High
- Press Program to device
- ECG chip should turn on, which can be seen with LEDs blinking

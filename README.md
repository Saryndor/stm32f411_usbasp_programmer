# STM32F411 Blackpill USBasp Programmer

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-STMF411_Blackpill-teal.svg)
![Protocol](https://img.shields.io/badge/protocol-USBasp-orange.svg)

## Table of Contents

- [Features](#features)
- [Hardware Setup](#hardware-setup)
- [Flashing firmware](#flashing-firmware-blackpill)
- [AVRDUDE Usage](#avrdude-usage)
- [Building the Project](#building-the-project)

---

A high-speed, native USB ISP programmer implementation for the STM32F411 Blackpill (stm32f411ceu6), emulating the popular **USBasp** protocol.

This project turns a Blackpill into a robust AVR In-System Programmer (ISP) capable of flashing generic AVR microcontrollers (ATtiny, ATmega) using `avrdude`.

---

## Features

- **Hybrid SPI Engine:**
  - **Hardware SPI:** For high-speed programming (up to 3 MHz).
  - **Software SPI:** Automatic fallback for low-speed targets (down to < 1 kHz) or rescue clocking.
  - **Default Speed:** 187.5 kHz
- **Smart Timing:** Implements logic based on Microchip/Atmel ATDF specifications for robust timing and stability.
- **Smart Target Isolation:** Automatically disables the level shifter outputs (High-Impedance) when the programmer is idle. This allows the programmer to **remain permanently connected** to the target circuit without interfering with its normal operation.
- **Integrated CDC Serial Bridge:** Provides a virtual serial port (USB CDC) to communicate with the target's UART (e.g., debug output) without needing an extra USB-TTL adapter.
- **Extended Protocol:** Supports extended addressing for large Flash (>128kB) and robust page-writing logic.
- **Double Buffered USB:** Efficient endpoint handling to maximize throughput.
- **UF2 Bootloader:** Simple way for firmware updates (Thanks to Adafruit)

## Hardware Setup


### Pin Mapping 

| Signal | Pin | Description |
| :--- | :--- | :--- |
| **MOSI** | PB15 | Master Out Slave In |
| **MISO** | PB14 | Master In Slave Out |
| **SCK** | PB10 | Serial Clock |
| **RST** | PB13 | Target Reset Control |
| **TX** | PA2 | UART2 TX (Connected to Target RX) |
| **RX** | PA3 | UART2 RX (Connected to Target TX)|
| **OE** | PA4 | Output Enable HCT245 |
| **MISO-PWR** | PA5 | MISO Pull-Up Power Control (Active during programming) |


### Recommended Circuit

To achieve stable communication at 3 MHz, the following protection and impedance matching circuit is implemented:

**1. Level Shifting & Isolation (3.3V Logic -> 5V Target)**

An **SN74HCT245** transceiver is used for signals going TO the target (MOSI, SCK, RST).

- **Level Shifting:** It shifts the 3.3V signals to the target's voltage level.
- **Isolation:** The firmware controls the **Output Enable (OE)** pin of the HCT245. When idle, the outputs are switched to high-impedance (tri-state), electrically disconnecting the programmer lines from the target.

**2. Protection (5V Target -> 3.3V Logic)**

A **BAT85 Schottky Diode** configuration protects the MISO input:

- **Diode:** `Cathode` to Blackpill MISO, `Anode` to Target MISO.
- **Pull-Up:** A **500Ω Pull-Up resistor** connects PA5 to Blackpill's MISO.
- **Logic:** The diode blocks the target's 5V. When the target pulls MISO to GND, the diode conducts and pulls the Blackpill pin to LOW. When the target is HIGH, the diode blocks, and the active pull-up on PA5 ensures a clean 3.3V HIGH signal.
- **Smart Isolation:** The pull-up is only energized during active programming sessions over PA5. This prevents any leakage current or interference when the programmer is idle, allowing the target to run undisturbed.

**3. Impedance Matching**
**100Ω series resistors** are placed on SCK, MOSI, and RST lines to dampen reflections (ringing) on the cable.

## Flashing Firmware (Blackpill)

### Activate Bootloader Mode (First Flash w/o UF2 Bootloader)

- **Option 1:**
   1. Hold down the **BOOT0** button.
   2. Press the **Reset** button for a second.
   3. Release the **Reset** button.
   4. Wait a second, then release the **BOOT0** button.
 
- **Option 2:**
   1. Disconnect the USB cable.
   2. Hold down the **BOOT0** button.
   3. Reconnect the USB cable.
   4. Wait a second, then release the **BOOT0** button.
   
When the Blackpill is in Bootloader Mode, you can use `sudo dfu-util -l` to verify the state.

> ⚠️ **Note:** If you're having trouble putting the Blackpill into bootloader mode, it might help to slightly warm up the PCB and **avoiding** a connection over a USB hub. It might also be an **advantage** to use a USB 2.0 port.
 
#### Flash firmware

```bash
sudo dfu-util -a 0 -s 0x08000000:leave -D bootloader_firmware.bin
```
-----

### Activate UF2 Bootloader Mode 

There are two choices to activate the uf2 bootloader.

  1. Press the reset button twice within 500 ms
  2. Change the baud rate to 1200 for the cdc port, e.g.
      ```bash
      stty -F /dev/ttyACM0 1200
      ```

### Upload UF2 Firmware File

Once activation is complete, a drive named `STM411BOOT` should appear.
Copy the file `firmware.uf2` to the drive and the flash process should start automatically.

---

### Linux Setup (udev rules)

To use the programmer without `root` privileges (without `sudo`), you must configure a udev rule. This is specifically required for Debian-based systems to grant your user access to the USB device.

1.  **Create the Rule File:**
    Create a new file in the udev rules directory:
    ```bash
    sudo nano /etc/udev/rules.d/99-usbasp.rules
    ```

2.  **Add the Configuration:**
    Paste the following line into the file. This targeting assumes the standard USBasp VID/PID (`16c0:05dc`):
    ```text
    SUBSYSTEM=="usb", ATTR{idVendor}=="16c0", ATTR{idProduct}=="05dc", MODE="0666", GROUP="dialout"
    ```

3.  **Reload udev Service:**
    Apply the new rules immediately without restarting the system:
    ```bash
    sudo udevadm control --reload-rules
    sudo udevadm trigger
    ```

4.  **Group Membership:**
    Ensure your user is a member of the `dialout` group:
    ```bash
    sudo usermod -a -G dialout $USER
    ```
    *Note: You may need to log out and back in for the group change to take effect.*


Now, re-plug your Blackpill. You should be able to run `avrdude` as a normal user.

---

## AVRDUDE Usage

#### Verify device

```bash
avrdude -c usbasp-clone -p m328p  -vv
```

```bash
Avrdude version 8.1
Copyright see https://github.com/avrdudes/avrdude/blob/main/AUTHORS

Using port            : usb
Using programmer      : usbasp-clone
Seen device from vendor >Saryndor Dev.<
Seen product >USBasp+CDC<
AVR part              : ATmega328P
Programming modes     : SPM, ISP, HVPP, debugWIRE

Memory           Size  Pg size
------------------------------
eeprom           1024        4
flash           32768      128
lfuse               1        1
hfuse               1        1
efuse               1        1
lock                1        1
prodsig/sigrow     24        1
sernum             10        1
io                224        1
sram             2048        1
signature           3        1
calibration         1        1

Variants         Package  F max   T range         V range       
----------------------------------------------------------------
ATmega328P       N/A      20 MHz  [-40 C,   N/A]  [1.8 V, 5.5 V]

Programmer type       : usbasp
Description           : Any usbasp clone with correct VID/PID
Auto set sck period

AVR device initialized and ready to accept instructions
Reading | ################################################## | 100% 0.00 s 
Device signature = 1E 95 0F (ATmega328P, ATA6614Q, LGT8F328P)

Avrdude done.  Thank you.
```

#### Read Flash (default speed)

```bash
avrdude -c usbasp-clone -p m328p -U flash:r:/tmp/backup_m328p.hex:i
```

```bash
Reading flash memory ...
Reading | ################################################## | 100% 6.05 s 
Writing 7198 bytes to output file backup_m328p.hex

Avrdude done.  Thank you.
```

#### Read Flash (full speed)

```bash
avrdude -c usbasp-clone -p m328p -U flash:r:/tmp/backup_m328p.hex:i -B 3Mhz
```

```bash
Set SCK frequency to 3000000 Hz
Reading flash memory ...
Reading | ################################################## | 100% 0.71 s 
Writing 7198 bytes to output file backup_m328p.hex

Avrdude done.  Thank you.
```

#### Write Flash (default speed)

```bash
avrdude -c usbasp-clone -p m328p -U flash:w:/tmp/backup_m328p.hex:i
```

```bash
Reading 7198 bytes for flash from input file backup_m328p.hex
Writing 7198 bytes to flash
Writing | ################################################## | 100% 1.93 s 
Reading | ################################################## | 100% 1.38 s 
7198 bytes of flash verified

Avrdude done.  Thank you.
```

#### Write Flash (full speed)

```bash
avrdude -c usbasp-clone -p m328p -U flash:w:/tmp/backup_m328p.hex:i -B 3Mhz
```

```bash
Set SCK frequency to 3000000 Hz
Reading 7198 bytes for flash from input file backup_m328p.hex
Writing 7198 bytes to flash
Writing | ################################################## | 100% 0.73 s 
Reading | ################################################## | 100% 0.17 s 
7198 bytes of flash verified

Avrdude done.  Thank you.
```

---

## Building the Project

> ⚠️ **Note:** The following instructions and requirements are designed for Debian-based systems. If you are using a different distribution or system (e.g., Fedora, AlmaLinux, Windows, Mac), you may need to adjust the package names or commands accordingly.

### Requirements

- **Toolchain:** `arm-none-eabi-gcc` (e.g. [arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi](https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz))
- **Utilities:** `cmake`, `ninja`, `openocd`
- **Container:** `podman`   
- **Programmers:** `stlink/v2`, `jlink`, `Black Magic Probe`, `dfu-util`

### Build Instructions

1.  **Clone repo:**

      ```bash
      git clone --recursive https://github.com/Saryndor/stm32f411_usbasp_programmer.git
      ```

2.  **Change directory:**

      ```bash
      cd stm32f411_usbasp_programmer
      ```

3.  **TinyUSB: Add STM32F4 dependencies**

      ```bash
      python3 lib/tinyusb/tools/get_deps.py stm32f4
      ```

4.  **Create & Start Container (Optional but recommended):**

      ```bash
      cd .podman
      ./build-image.sh
      ./run-container.sh
      podman exec -it c-f4-usbasp-eabi /bin/bash
      ```


5.  **Build Firmware (Inside Container):**

      ```bash
      make -C libopencm3 clean
      mkdir build && cmake -S . -B build -G Ninja --preset arm-none-eabi
      cmake --build build
      ```

      After a successful build you should see the firmware files as part of the `build` folder

      | File | Description | Usage |
      | :--- | :--- | :--- |
      | `firmware.elf` | Application with debug symbols | **Debugging** via GDB/SVD. |
      | `firmware.bin` | Raw application data | Base for the UF2 image. |
      | `firmware.uf2` | Application in UF2 format | **Update** via USB drive (Drag & Drop). |
      | `bootloader_firmware.bin` | Full image (Bootloader + App). | **Initial flash** new hardware |
      | `bootloader_firmware.elf` | Full image as ELF container | **Flash** via ST-Link / J-Link / Black Magic Probe. |


6.  **Uploading Firmware:**

      ```bash
      cmake --build build --target stlink-flash
      ```

      ```bash
      cmake --build build --target jlink-flash
      ```

      ```bash
      cmake --build build --target bmp-flash
      ```

7.   **Clean & (Re)Configure:** (when needed)

      ```bash
      make -C libopencm3 clean
      rm -rf build/* && cmake --preset arm-none-eabi
      ```

## License

This project is licensed under the [MIT License](LICENSE).


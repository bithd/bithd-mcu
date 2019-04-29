# BITHD Firmware

https://bithd.com/

## How to build BITHD firmware?

1. <a href="https://docs.docker.com/engine/installation/">Install Docker</a>
2. `git clone https://github.com/bithd/bithd-mcu.git`
3. `cd bithd-mcu`
4. `./build-firmware.sh TAG` (where TAG is v1.5.0 for example, if left blank the script builds latest commit in master branch)

This creates file `build/trezor-TAG.bin` and prints its fingerprint and size at the end of the build log.

## How to build BITHD bootloader?

1. <a href="https://docs.docker.com/engine/installation/">Install Docker</a>
2. `git clone https://github.com/bithd/bithd-mcu.git`
3. `cd bithd-mcu`
4. `./build-bootloader.sh TAG` (where TAG is bl1.3.2 for example, if left blank the script builds latest commit in master branch)

This creates file `build/bootloader-TAG.bin` and prints its fingerprint and size at the end of the build log.
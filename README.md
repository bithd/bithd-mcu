# BITHD Firmware

https://bithd.com/

## How to build BITHD firmware?

1. <a href="https://docs.docker.com/engine/installation/">Install Docker</a>
2. `git clone https://github.com/bithd/bithd-mcu.git`
3. `cd bithd-mcu`
4. `export TAG=v2.7.4; ./build-firmware.sh $TAG` (where TAG is v2.7.4 for example)
5. `pipenv --python 3 install` (set up pipenv)
6. `pipenv run ./script/prepare_firmware.py -f ./build/bithd-$TAG-unsigned.bin` (prepare the image to sign)

This creates file `build/bithd-$TAG-unsigned.bin` and prints its fingerprint and size at the end of the build log.

## How to build BITHD bootloader?

1. <a href="https://docs.docker.com/engine/installation/">Install Docker</a>
2. `git clone https://github.com/bithd/bithd-mcu.git`
3. `cd bithd-mcu`
4. `./build-bootloader.sh TAG` (where TAG is bl1.3.2 for example)

This creates file `build/bootloader-TAG.bin` and prints its fingerprint and size at the end of the build log.

## Verify signed image exported from App

1. Get the same version signed firmware image exported from App
2. Compare the unsigned and signed image files, eg v2.7.4
```shell
export TAG=v2.7.4
diff <(xxd build/bithd-$TAG-prepared.bin) <(xxd build/bithd-$TAG-signed.bin)
```
The following is the comparison result for v2.7.4.  The only differences are the first 256 bytes that the signed image has the signatures.
```
1c1
< 00000000: 5452 5a52 9433 0600 0000 0001 0000 0000  TRZR.3..........
---
> 00000000: 5452 5a52 9433 0600 0304 0501 0000 0000  TRZR.3..........
5,16c5,16
< 00000040: 0000 0000 0000 0000 0000 0000 0000 0000  ................
< 00000050: 0000 0000 0000 0000 0000 0000 0000 0000  ................
< 00000060: 0000 0000 0000 0000 0000 0000 0000 0000  ................
< 00000070: 0000 0000 0000 0000 0000 0000 0000 0000  ................
< 00000080: 0000 0000 0000 0000 0000 0000 0000 0000  ................
< 00000090: 0000 0000 0000 0000 0000 0000 0000 0000  ................
< 000000a0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
< 000000b0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
< 000000c0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
< 000000d0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
< 000000e0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
< 000000f0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
---
> 00000040: 3c47 3126 c938 aa6c 7bb0 eac0 27e7 0e7d  <G1&.8.l{...'..}
> 00000050: 522b eed1 e072 276c b6f9 44d9 6361 679f  R+...r'l..D.cag.
> 00000060: 8054 e65d ab27 1c1b 4637 5f12 d868 b41d  .T.].'..F7_..h..
> 00000070: 69a8 9c77 61c5 64c3 2d2c 8e73 5edc ff93  i..wa.d.-,.s^...
> 00000080: c174 884d 1fe3 5199 d18f 3411 5009 ee12  .t.M..Q...4.P...
> 00000090: 92e4 bf73 0e8b fc0e ccd7 e5cf 96ad 663a  ...s..........f:
> 000000a0: 5372 3b6c 7398 32b7 5bea 3657 9d7e 9028  Sr;ls.2.[.6W.~.(
> 000000b0: 7803 eeec 4eb4 7d9b bcd4 75c7 cb2f 8c4c  x...N.}...u../.L
> 000000c0: d7ab 90c4 c8b0 6776 0e92 320a 55a4 e623  ......gv..2.U..#
> 000000d0: fac5 806b bdc5 3120 b351 a283 9ee9 460c  ...k..1 .Q....F.
> 000000e0: e882 9add fc10 ac59 df00 39c4 fa92 041f  .......Y..9.....
> 000000f0: 88c0 47f9 80a5 19ae 15aa 7566 2c74 6798  ..G.......uf,tg.
```

## Build signed image

Based on the upper prepared firmware image, anyone can build the same signed image by signatures uploaded in code repository.

1. Use the build_signed_firmware.py script to generate the signed firmware image
```shell
export TAG=v2.7.4
pipenv run ./script/build_signed_firmware.py -f ./build/bithd-$TAG-prepared.bin -s ./signatures/$TAG.csv
```

2. `shasum -a 256 ./build/bithd-$TAG-signed.bin` (calculate sha256 checksum)

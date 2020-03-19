# BITHD Firmware

https://bithd.com/

## How to build BITHD firmware?

1. <a href="https://docs.docker.com/engine/installation/">Install Docker</a>
2. `git clone https://github.com/bithd/bithd-mcu.git`
3. `cd bithd-mcu`
4. `export TAG=v2.7.3; ./build-firmware.sh $TAG` (where TAG is v2.7.3 for example)
5. `./script/prepare_firmware.py -f ./build/bithd-$TAG-unsigned.bin` (prepare the image to sign)

This creates file `build/bithd-$TAG-unsigned.bin` and prints its fingerprint and size at the end of the build log.

## How to build BITHD bootloader?

1. <a href="https://docs.docker.com/engine/installation/">Install Docker</a>
2. `git clone https://github.com/bithd/bithd-mcu.git`
3. `cd bithd-mcu`
4. `./build-bootloader.sh TAG` (where TAG is bl1.3.2 for example)

This creates file `build/bootloader-TAG.bin` and prints its fingerprint and size at the end of the build log.

## Verify signed image exported from App

1. Get the same version signed firmware image exported from App
2. Compare the unsigned and signed image files, eg v2.7.3
```shell
export TAG=v2.7.3
diff <(xxd build/bithd-$TAG-prepared.bin) <(xxd signed/bithd-wallet-signed-$TAG.bin)
```
The following is the comparison result for v2.7.3.  The only differences are the first 256 bytes that the signed image has the signatures.
```shell script
1c1
< 00000000: 5452 5a52 2433 0600 0000 0001 0000 0000  TRZR$3..........
---
> 00000000: 5452 5a52 2433 0600 0304 0501 0000 0000  TRZR$3..........
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
> 00000040: 63d3 2d9a 8375 58f9 608a 37f1 2a8f 0797  c.-..uX.`.7.*...
> 00000050: eed3 2a5b 6525 d0cc 6bc6 3c55 d3af 6312  ..*[e%..k.<U..c.
> 00000060: a60f 00ae 6a4b f474 3473 a070 1585 1140  ....jK.t4s.p...@
> 00000070: f75f 8989 d825 a628 e29b 692f ded9 668c  ._...%.(..i/..f.
> 00000080: c365 7a17 e7b1 3154 2c44 78db 521b 24b7  .ez...1T,Dx.R.$.
> 00000090: 7ef7 67e9 0ad8 096c 1ff4 7c30 85c0 bd93  ~.g....l..|0....
> 000000a0: 8612 e0d0 c4d5 708a bb25 1ed1 9626 d10e  ......p..%...&..
> 000000b0: b516 3332 a6ce ab61 67c9 f99d ecbb 0eae  ..32...ag.......
> 000000c0: dfda c761 f46c 10cb ae09 8b61 4dcf 6d03  ...a.l.....aM.m.
> 000000d0: dbfb 14d8 10c5 0b6a 550a b79d 5b6d 590b  .......jU...[mY.
> 000000e0: 99e8 7a23 fe55 7c4e 09aa 893e 4eba 414d  ..z#.U|N...>N.AM
> 000000f0: 6bcb 878a f88e d953 7326 2991 1245 27df  k......Ss&)..E'.
```

## Build signed image

Based on the upper prepared firmware image, anyone can build the same signed image by signatures uploaded in code repository.

1. `pipenv --python 3 install` (set up pipenv)

2. Use the build_signed_firmware.py script to generate the signed firmware image
```shell
export TAG=v2.7.3
pipenv run ./script/build_signed_firmware.py -f ./build/bithd-$TAG-prepared.bin -s ./signatures/$TAG.csv
```

3. `shasum -a 256 ./build/bithd-$TAG-signed.bin` (calculate sha256 checksum)

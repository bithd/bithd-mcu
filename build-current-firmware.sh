#!/bin/bash
set -e

IMAGE=trezor-mcu-build
TAG=${1:-current}
BINFILE=build/bithd-$TAG-unsigned.bin
ELFFILE=build/bithd-$TAG.elf

echo "Build firmware for device $DEVICE_MODEL"

docker build -t $IMAGE .
docker run -e DEVICE_MODEL=${DEVICE_MODEL} -t -v "$(pwd)/build:/build:z" -v "$(pwd):/bithd-mcu" $IMAGE /bin/sh -c "\
	cd /bithd-mcu && \
	make clean && \
	make -C vendor/libopencm3 && \
	make -C vendor/nanopb/generator/proto && \
	make -C firmware/protob clean && \
	make -C firmware/protob && \
	make && \
	make -C firmware && \
	cp firmware/trezor.bin /$BINFILE && \
	cp firmware/trezor.elf /$ELFFILE"

/usr/bin/env python -c "
from __future__ import print_function
import hashlib
import sys
fn = sys.argv[1]
data = open(fn, 'rb').read()
print('\n\n')
print('Filename    :', fn)
print('Fingerprint :', hashlib.sha256(data).hexdigest())
print('Size        : %d bytes (out of %d maximum)' % (len(data), 491520))
" "$BINFILE"

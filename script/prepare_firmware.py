#!/usr/bin/env python
from __future__ import print_function
import argparse
import hashlib
import struct

SLOTS = 3


def get_arg_parser():
    parser = argparse.ArgumentParser(description='Commandline tool for signing BITHD firmware.')
    parser.add_argument('-f', '--file', required=True, dest='path', help="Firmware file to modify")

    return parser


def prepare(data):
    # Takes raw OR signed firmware and clean out metadata structure
    # This produces 'clean' data for signing

    meta = b'TRZR'  # magic
    if data[:4] == b'TRZR':
        meta += data[4:4 + struct.calcsize('<I')]
    else:
        meta += struct.pack('<I', len(data))  # length of the code
    meta += b'\x00' * SLOTS  # signature index #1-#3
    meta += b'\x01'       # flags
    meta += b'\x00' * 52  # reserved
    meta += b'\x00' * 64 * SLOTS  # signature #1-#3

    if data[:4] == b'TRZR':
        # Replace existing header
        out = meta + data[len(meta):]
    else:
        # create data from meta + code
        out = meta + data

    return out


def main():
    ap = get_arg_parser()
    args = ap.parse_args()

    data = open(args.path, 'rb').read()
    assert len(data) % 4 == 0

    if data[:4] != b'TRZR':
        print("Prepare to add metadata ...")
        data = prepare(data)

        if data[:4] != b'TRZR':
            raise Exception("Firmware header expected")
    else:
        print("Metadata has been added")

    print("Firmware size %d bytes" % len(data))

    fingerprint = hashlib.sha256(data[256:]).hexdigest()
    print("Firmware fingerprint:", fingerprint)

    out_path = args.path.replace("unsigned", "prepared")
    fp = open(out_path, 'wb')
    fp.write(data)
    fp.close()


if __name__ == '__main__':
    main()

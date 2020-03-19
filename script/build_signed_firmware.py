#!/usr/bin/env python
from __future__ import print_function
import argparse
import hashlib
import struct
import binascii
import ecdsa
import csv


pubkeys = {
    1: '048A6DEECF3C243CA373897A504B6910BE967CE511B7E08DBC2CB23B9A110F98E523A17ADDED4C2A133B2CBD7DF065EDC80425CAE71C90274469E17E0F631702D2',
    2: '04E4F37F1C2BEF3391D2D00171077410F5BB6802AB6E46406A9C4F834E733E2CB77D57F4CBF35F8592BDE201E64B4AC8C062ABB86E4512A4AF34DE6EE83A83B19F',
    3: '0468302E39022BA9DE1781A880ED0CF0741D5761BA534DE92F5BD8A884FBDD7FEB05D4808DF248A2161DABF789D8188897F32F40257F53E5AEF1211947F4DACC35',
    4: '041E61D0824D9896F57B6D5D0BC53B6E72A7D7CDFA92F3AF4B4891698939130B41879D98B9E024E4EAF3B154DEB2149927A07CF23C4340B8D2DD5FD595DFC71371',
    5: '04CF97F476D584DD2C0F61321599F1620CF4F11AF4CF6E0FBD1724A1608C4899DA6AA7C451C2F8AE3FEE924889F284AC4852EAADC644FA9B988ED2D3D13185D6F6',
}

SLOTS = 3
INDEXES_START = len('TRZR') + struct.calcsize('<I')
SIG_START = INDEXES_START + SLOTS + 1 + 52


def parse_args():
    parser = argparse.ArgumentParser(description='Commandline tool for adding signatures to BITHD firmware.')
    parser.add_argument('-f', '--file', required=True, dest='path', help="Firmware file to add signatures")
    parser.add_argument('-s', '--signatures', required=True, dest='sigs', help="Signature in csv files")

    return parser.parse_args()


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


def check_signatures(data):
    # Analyses given firmware and prints out
    # status of included signatures

    try:
        indexes = [ ord(x) for x in data[INDEXES_START:INDEXES_START + SLOTS] ]
    except:
        indexes = [ x for x in data[INDEXES_START:INDEXES_START + SLOTS] ]

    to_sign = prepare(data)[256:]  # without meta
    fingerprint = hashlib.sha256(to_sign).hexdigest()

    print("Firmware fingerprint:", fingerprint)

    used = []
    valid = True
    for x in range(SLOTS):
        signature = data[SIG_START + 64 * x:SIG_START + 64 * x + 64]

        if indexes[x] == 0:
            print("Slot #%d" % (x + 1), 'is empty')
        else:
            pk = pubkeys[indexes[x]]
            verify = ecdsa.VerifyingKey.from_string(
                binascii.unhexlify(pk)[1:], curve=ecdsa.curves.SECP256k1, hashfunc=hashlib.sha256)

            try:
                verify.verify(signature, to_sign, hashfunc=hashlib.sha256)

                if indexes[x] in used:
                    print("Slot #%d signature: DUPLICATE" % (x + 1), binascii.hexlify(signature))
                else:
                    used.append(indexes[x])
                    print("Slot #%d signature: VALID" % (x + 1), binascii.hexlify(signature))

            except:
                print("Slot #%d signature: INVALID" % (x + 1), binascii.hexlify(signature))
                valid = False
    return valid


def modify(data, slot, index, signature):
    data = bytearray(data)
    # put index to data
    data[INDEXES_START + slot - 1] = index
    # put signature to data
    data[SIG_START + 64 * (slot - 1): SIG_START + 64 * slot] = signature
    return bytes(data)


def main(args):
    print("Add signatures to BITHD firmware")

    data = open(args.path, 'rb').read()
    if data[:4] != b'TRZR':
        print("Metadata has been added...")
        data = prepare(data)

    if data[:4] != b'TRZR':
        raise Exception("Firmware header expected")

    with open(args.sigs, newline='') as csv_file:
        signatures_reader = csv.DictReader(csv_file)
        i = 1
        for row in signatures_reader:
            slot = int(row['slot'])
            signature = row['signature']
            data = modify(data, i, slot, bytes.fromhex(signature))
            print("Data saved:", binascii.hexlify(data[SIG_START:SIG_START + 192]))
            if i >= 3:
                break
            i = i + 1

    # for i in range(1, 4):
    #     slot = int(raw_input('Enter signature slot (1-%d): ' % 5))
    #
    #     data = modify(data, i, slot, bytes.fromhex(signingbuf[slot]))
    #     print("Data saved:", binascii.hexlify(data[SIG_START:SIG_START + 192]))

    if check_signatures(data):
        out_path = args.path.replace("prepared", "signed")
        fp = open(out_path, 'wb')
        fp.write(data)
        fp.close()
    else:
        print("Has invalid signature")


if __name__ == '__main__':
    main(parse_args())

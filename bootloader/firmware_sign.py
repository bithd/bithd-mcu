#!/usr/local/bin/python
from __future__ import print_function
import argparse
import hashlib
import struct
import binascii
import ecdsa

try:
    raw_input
except:
    raw_input = input

SLOTS = 3

signingbuf={
     
    1:'A5162EDF9D0D0A21EE9237D0D15F1AA6B387DE69AEF370BC67BBCC4D587F2A101240BDB8EBDB27208F28A599486E774DB99CDEA71C9DEDF4CB80FD8E7E48A9F2',
    2:'3736AB760CBA80DD1DC032ACD7AE59AE751B1B08A3466D52A40B79BD6868D21C9607478CDC7FE4E15F856004B7A44C9A4E590B9D10E3713434C1AB7D2C944EC4',
    3:'AC6B3193F2604FE98081983F0DF4C7594D4F4561929E97A7B69E86230EA0537A4EF8325421FB87D5236C00E023323F971ED5A54D5676E7726C7BAAFDE483A2B5',
    4:'ACCCF394D35167F583146ACDFF6C2AABFA0CAF017009601884D1F70D056C745F6F4FF39587664D7DCF209F2B7AA44C96B651066CEAD7CA9E0A005A58BC34F342',
    5:'9D57733EEC29BC95A895C5F5D62BCE095E9EC3A8CF35844523945A7CC42C1A2F4CE329C9CF8F8941C5FD91D35F2375920FEF161D1B246DED5A5E7304CC2C21CC',
}

pubkeys = {
    1: '048A6DEECF3C243CA373897A504B6910BE967CE511B7E08DBC2CB23B9A110F98E523A17ADDED4C2A133B2CBD7DF065EDC80425CAE71C90274469E17E0F631702D2',
    2: '04E4F37F1C2BEF3391D2D00171077410F5BB6802AB6E46406A9C4F834E733E2CB77D57F4CBF35F8592BDE201E64B4AC8C062ABB86E4512A4AF34DE6EE83A83B19F',
    3: '0468302E39022BA9DE1781A880ED0CF0741D5761BA534DE92F5BD8A884FBDD7FEB05D4808DF248A2161DABF789D8188897F32F40257F53E5AEF1211947F4DACC35',
    4: '041E61D0824D9896F57B6D5D0BC53B6E72A7D7CDFA92F3AF4B4891698939130B41879D98B9E024E4EAF3B154DEB2149927A07CF23C4340B8D2DD5FD595DFC71371',
    5: '04CF97F476D584DD2C0F61321599F1620CF4F11AF4CF6E0FBD1724A1608C4899DA6AA7C451C2F8AE3FEE924889F284AC4852EAADC644FA9B988ED2D3D13185D6F6',
}

INDEXES_START = len('TRZR') + struct.calcsize('<I')
SIG_START = INDEXES_START + SLOTS + 1 + 52

def parse_args():
    parser = argparse.ArgumentParser(description='Commandline tool for signing Trezor firmware.')
    parser.add_argument('-f', '--file', dest='path', help="Firmware file to modify")
    parser.add_argument('-s', '--sign', dest='sign', action='store_true', help="Add signature to firmware slot")
    parser.add_argument('-p', '--pem', dest='pem', action='store_true', help="Use PEM instead of SECEXP")
    parser.add_argument('-g', '--generate', dest='generate', action='store_true', help='Generate new ECDSA keypair')

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

    to_sign = prepare(data)[256:] # without meta
    fingerprint = hashlib.sha256(to_sign).hexdigest()

    print("Firmware fingerprint:", fingerprint)

    used = []
    for x in range(SLOTS):
        signature = data[SIG_START + 64 * x:SIG_START + 64 * x + 64]

        if indexes[x] == 0:
            print("Slot #%d" % (x + 1), 'is empty')
        else:
            pk = pubkeys[indexes[x]]
            verify = ecdsa.VerifyingKey.from_string(binascii.unhexlify(pk)[1:],
                        curve=ecdsa.curves.SECP256k1, hashfunc=hashlib.sha256)

            try:
                verify.verify(signature, to_sign, hashfunc=hashlib.sha256)

                if indexes[x] in used:
                    print("Slot #%d signature: DUPLICATE" % (x + 1), binascii.hexlify(signature))
                else:
                    used.append(indexes[x])
                    print("Slot #%d signature: VALID" % (x + 1), binascii.hexlify(signature))

            except:
                print("Slot #%d signature: INVALID" % (x + 1), binascii.hexlify(signature))


def modify(data, slot, index, signature):
    # Replace signature in data
    # Put index to 
    print("signature:")
    print(binascii.hexlify(signature))
    data = data[:INDEXES_START + slot - 1 ] + chr(index) + data[INDEXES_START + slot:]

    # Put signature to data
    data = data[:SIG_START + 64 * (slot - 1) ] + signature + data[SIG_START + 64 * slot:]
    print("slot:",slot)
    print("data:",binascii.hexlify(data[SIG_START:SIG_START+192]))
    return data

def sign(data, is_pem):
    # Ask for index and private key and signs the firmware

    slot = int(raw_input('Enter signature slot (1-%d): ' % SLOTS))
    if slot < 1 or slot > SLOTS:
        raise Exception("Invalid slot")

    if is_pem:
        print("Paste ECDSA private key in PEM format and press Enter:")
        print("(blank private key removes the signature on given index)")
        pem_key = ''
        while True:
            key = raw_input()
            pem_key += key + "\n"
            if key == '':
                break
        if pem_key.strip() == '':
            # Blank key,let's remove existing signature from slot
            return modify(data, slot, 0, '\x00' * 64)
        key = ecdsa.SigningKey.from_pem(pem_key)
    else:
        print("Paste SECEXP (in hex) and press Enter:")
        print("(blank private key removes the signature on given index)")
        secexp = raw_input()
        if secexp.strip() == '':
            # Blank key,let's remove existing signature from slot
            return modify(data, slot, 0, '\x00' * 64)
        key = ecdsa.SigningKey.from_secret_exponent(secexp = int(secexp, 16), curve=ecdsa.curves.SECP256k1, hashfunc=hashlib.sha256)

    to_sign = prepare(data)[256:] # without meta

    # Locate proper index of current signing key
    pubkey = b'04' + binascii.hexlify(key.get_verifying_key().to_string())
    index = None
    for i, pk in pubkeys.items():
        if pk == pubkey:
            index = i
            break

    if index == None:
        raise Exception("Unable to find private key index. Unknown private key?")

    signature = key.sign_deterministic(to_sign, hashfunc=hashlib.sha256)
    print("slot",slot)
    print("index",index)
    return modify(data, slot, index, signature)

def main(args):
    if args.generate:
        print("BITHD produce")
        
        for i in range(1,4):
            slot = int(raw_input('Enter signature slot (1-%d): ' % 5))

            data = open(args.path, 'rb').read()
            if data[:4] != b'TRZR':
                print("Metadata has been added...")
                data = prepare(data)

            if data[:4] != b'TRZR':
                raise Exception("Firmware header expected")
            data =modify(data, i, slot, signingbuf[slot].decode("hex"))
            print("datasave:",binascii.hexlify(data[SIG_START:SIG_START+320]))
            fp = open(args.path, 'wb')
            fp.write(data)
            fp.close()


        #for slot in range(3,6):
        #    data = open(args.path, 'rb').read()
        #    if data[:4] != b'TRZR':
        #        print("Metadata has been added...")
        #        data = prepare(data)

        #    if data[:4] != b'TRZR':
        #        raise Exception("Firmware header expected")
        #    data =modify(data, slot, slot, signingbuf[slot].decode("hex"))
        #    print("datasave:",binascii.hexlify(data[SIG_START:SIG_START+192]))
        #    fp = open(args.path, 'wb')
        #    fp.write(data)
        #    fp.close()


        #key = ecdsa.SigningKey.generate(
        #    curve=ecdsa.curves.SECP256k1,
        #    hashfunc=hashlib.sha256)

        #print("PRIVATE KEY (SECEXP):")
        #print(binascii.hexlify(key.to_string()))
        #print()

        #print("PRIVATE KEY (PEM):")
        #print(key.to_pem())

        #print("PUBLIC KEY:")
        #print('04' + binascii.hexlify(key.get_verifying_key().to_string()))
        return

    if not args.path:
        raise Exception("-f/--file is required")

    data = open(args.path, 'rb').read()
    print(args.path)
    assert len(data) % 4 == 0

    if data[:4] != b'TRZR':
        print("Metadata has been added...")
        data = prepare(data)

    if data[:4] != b'TRZR':
        raise Exception("Firmware header expected")

    print("Firmware size %d bytes" % len(data))

    check_signatures(data)

    if args.sign:
        data = sign(data, args.pem)
        check_signatures(data)

    fp = open(args.path, 'wb')
    fp.write(data)
    fp.close()

if __name__ == '__main__':
    args = parse_args()
    chose = int(input("chose porduce/debug (1/0):"))
    if chose == 1:
        args.generate=1
    main(args)

#!/usr/bin/python3

import hashlib, os, struct, sys

in_filename = sys.argv[1]
out_filename = sys.argv[2]

sha = hashlib.sha256()
with open(sys.argv[2], 'wb') as outfile:
    with open(sys.argv[1], 'rb') as infile:
        indata = infile.read()
        sha.update(indata)
        outfile.seek(128)
        outfile.write(indata)
        # Align to multiple of 128
        outfile.seek(((len(indata) + 255) & ~127) - 1)
        outfile.write(struct.pack('x'))
    print("SHA256: {0}".format(sha.hexdigest()))
    outfile.seek(0)
    outfile.write(struct.pack('<ccccH', b'M', b'C', b'U', b'0', int((len(indata) + 127) / 128)))
    outfile.seek(0x60)
    outfile.write(sha.digest())

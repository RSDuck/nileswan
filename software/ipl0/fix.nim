import os, strutils
const
    PayloadSize = 512

if paramCount() != 3 and paramStr(1) notin ["fullrom", "fpga"]:
    echo "usage: fix [fullrom|fpga] output input"
    echo "to either pad to 512 KB (fullrom) to test in an emulator (output is written as binary)"
    echo "or to generate a 512 byte snippet (fpga) to be run off the FPGA (output is written as ASCII hex file)"
    quit(1)
else:
    let
        fullrom = paramStr(1) == "fullrom"
        outputFile = paramStr(2)
        inputFile = paramStr(3)

        data = readFile(inputFile)
    if data.len != PayloadSize:
        echo "payload needs to be", PayloadSize, " bytes"
        quit(1)
    var output = newSeq[byte](if fullrom: 512*1024-PayloadSize else: 0)
    output.add(toOpenArrayByte(data, 0, high(data)))
    if not fullrom:
        # pad to 512 byte
        for i in 0..<output.len div 2:
            swap output[i*2], output[i*2+1]
        for i in 0..<512-PayloadSize:
            output.add 0xFF
        var asciiHex = newString(0)
        for i in 0..<output.len div 2:
            asciiHex &= toHex(output[i*2])
            asciiHex &= toHex(output[i*2+1])
            asciiHex &= '\n'
        writeFile(outputFile, asciiHex)
    else:
        writeFile(outputFile, output)

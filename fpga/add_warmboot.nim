import streams, os, strutils

const HeaderSize = 32

proc writeHeader(stream: FileStream, offset: uint32) =
    # https://github.com/YosysHQ/icestorm/blob/master/icemulti/icemulti.cc#L125

    # preamble
    stream.write 0x7e'u8
    stream.write 0xaa'u8
    stream.write 0x99'u8
    stream.write 0x7e'u8

    # boot mode
    stream.write 0x92'u8
    stream.write 0x00'u8
    stream.write 0x00'u8 # disable coldboot

    # boot address
    stream.write 0x44'u8
    stream.write 0x03'u8
    stream.write uint8(offset shr 16)
    stream.write uint8(offset shr 8)
    stream.write uint8(offset)

    # bank offset
    stream.write 0x82'u8
    stream.write 0x00'u8
    stream.write 0x00'u8

    # reboot
    stream.write 0x01'u8
    stream.write 0x08'u8

    # pad
    while (stream.getPosition() mod HeaderSize) != 0:
        stream.write(0x00'u8)

if paramCount() != 6:
    echo "usage: add_warmboot [first image] [warmboot 0 addr] [warmboot 1 addr] [warmboot 2 addr] [warmboot 3 addr] [output image]"
else:
    var warmBootImageAddrs: array[4, uint32]
    for i in 0..<4:
        warmBootImageAddrs[i] = uint32(parseHexInt(paramStr(2+i)))

    let
        inputImage = newFileStream(paramStr(1), fmRead)
        outputData = newFileStream(paramStr(6), fmWrite)

    # factory image, directly after the headers
    outputData.writeHeader(5'u32*HeaderSize)
    # boot image
    for i in 0'u32..<4:
        outputData.writeHeader(warmBootImageAddrs[i])

    outputData.write(inputImage.readAll())

    outputData.close()
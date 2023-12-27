# FPGA

- `rtl`: "Gateware" for the iCE40 FPGA on the nileswan
- `ipl0`: the first boot stage running on the FPGA, thus it has to be embedded into bitstream

## IPL0

256-byte program stored as the initial value of one EBRAM block. While each EBRAM block is technically 512 bytes in size only the lower 8 bits of the address bus are connected to the FPGA.

## Building

```
make
```
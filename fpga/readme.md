# FPGA

- `rtl`: "Gateware" for the iCE40 FPGA on the nileswan
- `ipl0`: the first boot stage running on the FPGA, thus it has to be embedded into bitstream

## IPL0

512-byte program stored as the initial value of one EBRAM block which is mapped at bank 500 and bank 511 to boot from it on Pocket Challange V2 or Wonderswan respectively.

## Building

```
make
```

# SPI flash layout

## Write-protected area

|  Start   |   End    | Description  |
|----------|----------|--------------|
| 0x000000 | 0x007FFF | FPGA core 0 (factory) + header |
| 0x008000 | 0x00BFFF | IPL1 (factory) |
| 0x00C000 | 0x00FFFF | IPL1 (recovery) |
| 0x010000 | 0x03FFFF | Recovery software (factory) |

## Write-unprotected area

|  Start   |   End    | Description  |
|----------|----------|--------------|
| 0x040000 | 0x043FFF | IPL1 (current) |
| 0x044000 | 0x04FFFF | Reserved |
| 0x050000 | 0x07FFFF | Recovery software (current) |
| 0x080000 | 0x087FFF | FPGA core 1 |
| 0x088000 | 0x08FFFF | FPGA core 2 |
| 0x090000 | 0x097FFF | FPGA core 3 |
| 0x098000 | 0x09FFFF | FPGA core 4 |
| 0x0A0000 | 0x1FFFFF | Unused (future expansion) |

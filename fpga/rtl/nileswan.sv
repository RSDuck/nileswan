`include "dance.sv"
`include "bootrom.sv"
`include "spi.sv"
`include "ipcram.sv"
`include "eeprom.sv"

module nileswan(
    input nSel, input nOE, input nWE,
    input nIO,

    input[8:0] AddrLo, input[3:0] AddrHi,
    inout[15:0] Data,

    output[6:0] AddrExt,
    output nMem_OE, nMem_WE,
    output nPSRAM1Sel, output nPSRAM2Sel,
    output PSRAM_nLB, PSRAM_nUB,
    output nSRAMSel,

    output MBC,
    input SClk,

    output nCartInt,

    input FastClk, output FastClkEnable,

    output nMCUSel,
    output nFlashSel,
    output SPIClk,
    output SPIDo,
    inout SPIDi,

    output nTFSel,
    output TFClk,
    output TFDo,
    input TFDi,
    
    output TFPow,
    
    output nMCUReset);

    // POW_CNT
    reg enable_fastclk = 1'b1;
    assign FastClkEnable = ~enable_fastclk;
    reg enable_tf_power = 1'b0;
    assign TFPow = enable_tf_power;
    reg nmcu_reset = 1'b1;
    assign nMCUReset = nmcu_reset;
    reg enable_nileswan_ex = 1'b1;
    reg enable_bandai2001_ex = 1'b1;
    reg enable_bandai2003_ex = 1'b1;

    reg[1:0] eeprom_size = 2'h2;

    assign nMem_OE = nOE;
    assign nMem_WE = nWE;

    assign nCartInt = 1'b1;

    Dance dance (
        .AddrLo(AddrLo[7:0]),
        .AddrHi(AddrHi),
        .SClk(SClk),
        .MBC(MBC));

    wire IOWrite = ~nSel & ~nIO;
    wire[7:0] RegAddr = {AddrHi, AddrLo[3:0]};

    wire write_txbuf;
    wire[15:0] rxbuf_read;
    wire[15:0] spi_cnt;
    reg write_spi_cnt_lo, write_spi_cnt_hi;
    SPI spi (
        .FastClk(FastClk),
        .SClk(SClk),
        .nWE(nWE), .nOE(nOE),
        
        .BufAddr(AddrLo),
        .WriteData(Data[7:0]),
        
        .RXBufData(rxbuf_read),
        .SPICnt(spi_cnt),
        
        .WriteSPICntLo(write_spi_cnt_lo & IOWrite),
        .WriteSPICntHi(write_spi_cnt_hi & IOWrite),
        .WriteTXBuffer(write_txbuf),

        .SPIDo(SPIDo),
        .SPIDi(SPIDi),
        .SPIClk(SPIClk),
        .nFlashSel(nFlashSel),
        .nMCUSel(nMCUSel),

        .TFPow(enable_tf_power),

        .TFDo(TFDo),
        .TFDi(TFDi),
        .TFClk(TFClk),
        .nTFSel(nTFSel));

    reg sel_serial_ctrl;
    reg sel_serial_com_lo, sel_serial_com_hi;
    reg sel_serial_data_lo, sel_serial_data_hi;

    wire[7:0] serial_ctrl;
    wire[15:0] serial_com, serial_data;

    EEPROM eeprom (
        .SClk(SClk),
        .nWE(nWE),
        .nOE(nOE),

        .EEPROMSize(eeprom_size),

        .WriteData(Data[7:0]),

        .SelSerialCtrl(sel_serial_ctrl),
        .SelSerialComLo(sel_serial_com_lo),
        .SelSerialComHi(sel_serial_com_hi),
        .SelSerialDataLo(sel_serial_data_lo),
        .SelSerialDataHi(sel_serial_data_hi),

        .SerialCtrl(serial_ctrl),
        .SerialCom(serial_com),
        .SerialData(serial_data)
    );

    reg[9:0] ram_addr_ext = 10'h3FF;
    reg[9:0] rom0_addr_ext = 10'h3FF;
    reg[9:0] rom1_addr_ext = 10'h3FF;
    reg[7:0] rom_linear_addr_ext = 8'hFF;

    reg self_flash = 1'h0;

    reg[8:0] rom_bank_mask = 9'h1FF;
    reg[3:0] ram_bank_mask = 8'hF;
    reg bank_mask_apply_rom_0 = 1'b1;
    reg bank_mask_apply_rom_1 = 1'b1;
    reg bank_mask_apply_ram = 1'b1;

    reg[7:0] reg_out = 0;
    reg reg_ack;

    // Bandai 2001 chip
    localparam LINEAR_ADDR_OFF = 8'hC0;
    localparam RAM_BANK = 8'hC1;
    localparam ROM_BANK_0 = 8'hC2;
    localparam ROM_BANK_1 = 8'hC3;

    localparam CART_SERIAL_DATA_L = 8'hC4;
    localparam CART_SERIAL_DATA_H = 8'hC5;
    localparam CART_SERIAL_COM_L = 8'hC6;
    localparam CART_SERIAL_COM_H = 8'hC7;
    localparam CART_SERIAL_CTRL = 8'hC8;

    // Bandai 2003
    localparam MEMORY_CTRL = 8'hCE;
    localparam RAM_BANK_L = 8'hD0;
    localparam RAM_BANK_H = 8'hD1;
    localparam ROM_BANK_0_L = 8'hD2;
    localparam ROM_BANK_0_H = 8'hD3;
    localparam ROM_BANK_1_L = 8'hD4;
    localparam ROM_BANK_1_H = 8'hD5;

    // nileswan extension
    localparam BANK_MASK_LO = 8'hE4;
    localparam BANK_MASK_HI = 8'hE5;

    localparam SPI_CNT_LO = 8'hE0;
    localparam SPI_CNT_HI = 8'hE1;

    localparam POW_CNT = 8'hE2;

    `define read2003Reg(value) \
        begin \
            reg_out = ``value``; \
            reg_ack = 1; \
        end
    `define readNileReg(value) \
        begin \
            reg_out = ``value``; \
            reg_ack = 1; \
        end
    
    `define readExternalReg(value, ext, enable) \
        begin \
            reg_out = ``value``; \
            ``ext`` = 1; \
            reg_ack = 1; \
        end

    wire[7:0] PowCnt = {nmcu_reset,
                eeprom_size,
                enable_bandai2003_ex,
                enable_bandai2001_ex,
                enable_nileswan_ex,
                enable_tf_power,
                enable_fastclk};


    always_comb begin
        reg_ack = 1;
        reg_out = 0;

        write_spi_cnt_lo = 0;
        write_spi_cnt_hi = 0;

        sel_serial_ctrl = 0;
        sel_serial_com_lo = 0;
        sel_serial_com_hi = 0;
        sel_serial_data_lo = 0;
        sel_serial_data_hi = 0;

        case (RegAddr)
        LINEAR_ADDR_OFF: reg_out = rom_linear_addr_ext;
        RAM_BANK: reg_out = ram_addr_ext[7:0];
        ROM_BANK_0: reg_out = rom0_addr_ext[7:0];
        ROM_BANK_1: reg_out = rom1_addr_ext[7:0];
        
        RAM_BANK_L: `read2003Reg(ram_addr_ext[7:0])
        ROM_BANK_0_L: `read2003Reg(rom0_addr_ext[7:0])
        RAM_BANK_H: `read2003Reg({6'h0, ram_addr_ext[9:8]})
        ROM_BANK_0_H: `read2003Reg({6'h0, rom0_addr_ext[9:8]})
        ROM_BANK_1_H: `read2003Reg({6'h0, rom1_addr_ext[9:8]})
        MEMORY_CTRL: `read2003Reg({7'h0, self_flash})

        CART_SERIAL_DATA_L:
            `readExternalReg(serial_data[7:0], sel_serial_data_lo, enable_bandai2001_ex)
        CART_SERIAL_DATA_H:
            `readExternalReg(serial_data[15:8], sel_serial_data_hi, enable_bandai2001_ex)
        CART_SERIAL_COM_L:
            `readExternalReg(serial_com[7:0], sel_serial_com_lo, enable_bandai2001_ex)
        CART_SERIAL_COM_H:
            `readExternalReg(serial_com[15:8], sel_serial_com_hi, enable_bandai2001_ex)
        CART_SERIAL_CTRL:
            `readExternalReg(serial_ctrl, sel_serial_ctrl, enable_bandai2001_ex)

        BANK_MASK_LO: `readNileReg(rom_bank_mask[7:0])
        BANK_MASK_HI: `readNileReg({ram_bank_mask,
            bank_mask_apply_ram,
            bank_mask_apply_rom_1,
            bank_mask_apply_rom_0,
            rom_bank_mask[8]})
        SPI_CNT_LO: `readExternalReg(spi_cnt[7:0], write_spi_cnt_lo, enable_nileswan_ex)
        SPI_CNT_HI: `readExternalReg(spi_cnt[15:8], write_spi_cnt_hi, enable_nileswan_ex)

        POW_CNT: `readNileReg(PowCnt)
        default: reg_ack = 0;
        endcase
    end

    always @(posedge nWE) begin
        if (IOWrite) begin
            case (RegAddr)
            LINEAR_ADDR_OFF: rom_linear_addr_ext <= Data;
            RAM_BANK: ram_addr_ext[7:0] <= Data;
            ROM_BANK_0: rom0_addr_ext[7:0] <= Data;
            ROM_BANK_1: rom1_addr_ext[7:0] <= Data;

            RAM_BANK_L: if (enable_bandai2003_ex) ram_addr_ext[7:0] <= Data;
            ROM_BANK_0_L: if (enable_bandai2003_ex) rom0_addr_ext[7:0] <= Data;
            ROM_BANK_1_L: if (enable_bandai2003_ex) rom1_addr_ext[7:0] <= Data;
            RAM_BANK_H: if (enable_bandai2003_ex) ram_addr_ext[9:8] <= Data[1:0];
            ROM_BANK_0_H: if (enable_bandai2003_ex) rom0_addr_ext[9:8] <= Data[1:0];
            ROM_BANK_1_H: if (enable_bandai2003_ex) rom1_addr_ext[9:8] <= Data[1:0];
            MEMORY_CTRL: if (enable_bandai2003_ex) self_flash <= Data[0];

            BANK_MASK_LO: if (enable_nileswan_ex) rom_bank_mask[7:0] <= Data;
            BANK_MASK_HI: begin
                if (enable_nileswan_ex) begin
                    rom_bank_mask[8] <= Data[0];
                    bank_mask_apply_rom_0 = Data[1];
                    bank_mask_apply_rom_1 = Data[2];
                    bank_mask_apply_ram = Data[3];
                    ram_bank_mask <= Data[7:4];
                end
            end

            POW_CNT: begin
                if (enable_nileswan_ex || Data[7:0] == 8'hFD) begin
                    enable_fastclk <= Data[0];
                    enable_tf_power <= Data[1];

                    // disabled until it is ensured software handles then correctly
                    /*enable_nileswan_ex <= Data[2];
                    enable_bandai2001_ex <= Data[3];
                    enable_bandai2003_ex <= Data[4];*/

                    eeprom_size <= Data[6:5];

                    nmcu_reset <= Data[7];
                end
            end

            default: begin end
            endcase
        end
    end

    reg[8:0] rom_addr_ext_fin;
    reg sel_rom_space, sel_ram_space;
    reg access_in_ram_area;
    reg apply_bank_mask;
    always_comb begin
        rom_addr_ext_fin = 9'hXX;
        sel_rom_space = 0;
        sel_ram_space = 0;
        access_in_ram_area = 0;
        apply_bank_mask = 1;
        case (AddrHi)
            4'h0: begin end
            4'h1: begin
                sel_rom_space = self_flash;
                // if the RAM mask is 0 and mask is applied to SRAM
                sel_ram_space = ~self_flash & (ram_bank_mask != 4'h0) & bank_mask_apply_ram;
                rom_addr_ext_fin = ram_addr_ext[8:0];
                access_in_ram_area = 1;
                apply_bank_mask = bank_mask_apply_ram;
            end
            4'h2: begin
                sel_rom_space = 1;
                rom_addr_ext_fin = rom0_addr_ext[8:0];
                apply_bank_mask = bank_mask_apply_rom_0;
            end
            4'h3: begin
                sel_rom_space = 1;
                rom_addr_ext_fin = rom1_addr_ext[8:0];
                apply_bank_mask = bank_mask_apply_rom_1;
            end
            default: begin
                sel_rom_space = 1;
                rom_addr_ext_fin = {rom_linear_addr_ext[4:0], AddrHi};
            end
        endcase
    end

    wire[8:0] addr_ext_masked_rom =  apply_bank_mask ? (rom_addr_ext_fin & rom_bank_mask) : rom_addr_ext_fin;
    // SRAM address space is always mapped via RAM_BANK
    // while ROM address space depends on where
    wire[3:0] addr_ext_masked_ram = apply_bank_mask ? (ram_addr_ext[3:0] & ram_bank_mask) : ram_addr_ext[3:0];

    wire psram_1_addr = sel_rom_space && addr_ext_masked_rom[8:7] == 2'h0;
    wire psram_2_addr = sel_rom_space && addr_ext_masked_rom[8:7] == 2'h1;
    wire rxbuf_addr = sel_rom_space && addr_ext_masked_rom == 9'h1FE;
    wire bootrom_addr = sel_rom_space && (addr_ext_masked_rom == 9'h1FF || addr_ext_masked_rom == 9'h1F4);
    
    wire sram_addr = sel_ram_space && addr_ext_masked_ram[3] == 1'h0;
    wire ipcbuf_addr = sel_ram_space && addr_ext_masked_ram == 4'hE;
    wire txbuf_addr = sel_ram_space && addr_ext_masked_ram == 4'hF;

    assign AddrExt[2:0] = sel_rom_space ? addr_ext_masked_rom[2:0] : addr_ext_masked_ram[2:0];
    // save some LEs, SRAM ignores the banking bits above bit 2
    assign AddrExt[6:3] = addr_ext_masked_rom[6:3];

    assign nPSRAM1Sel = ~(~nSel & nIO & psram_1_addr);
    assign nPSRAM2Sel = ~(~nSel & nIO & psram_2_addr);
    assign nSRAMSel = ~(~nSel & nIO & sram_addr);

    assign PSRAM_nLB = access_in_ram_area & AddrLo[0];
    assign PSRAM_nUB = access_in_ram_area & ~AddrLo[0];

    assign write_txbuf = ~nSel & nIO & txbuf_addr;

    wire[15:0] bootrom_read;
    BootROM blockram (
        .nOE(nOE),
        .AddrLo(AddrLo),
        .Sel(~nSel & nIO & bootrom_addr),
        .ReadData(bootrom_read));

    wire[7:0] ipc_read;
    IPCRAM ipcram (
        .nOE(nOE),
        .nWE(nWE),
        .Sel(~nSel & nIO & ipcbuf_addr),
        .AddrLo(AddrLo),
        .ReadData(ipc_read),
        .WriteData(Data[7:0]));

    wire sel_oe = ~nSel & ~nOE;

    wire psram_hi_write = access_in_ram_area & ~(nPSRAM1Sel & nPSRAM2Sel) & ~nWE & AddrLo[0];
    wire psram_hi_read = access_in_ram_area & ~(nPSRAM1Sel & nPSRAM2Sel) & ~nOE & AddrLo[0];

    wire output_io_out = ~nIO & reg_ack;
    wire output_bootrom = nIO & bootrom_addr;
    wire output_ipcbuf = nIO & ipcbuf_addr;
    wire output_rxbuf = nIO & rxbuf_addr;

    wire enable_output_lo = (sel_oe & (output_io_out | output_bootrom | output_ipcbuf | output_rxbuf)) | psram_hi_read;
    wire enable_output_hi = (sel_oe & (output_bootrom | output_rxbuf)) | psram_hi_write;

    reg[7:0] output_data_lo, output_data_hi;
    always_comb begin
        casez ({output_ipcbuf, psram_hi_read, output_io_out, output_bootrom, output_rxbuf})
        5'b1????: output_data_lo = ipc_read;
        5'b?1???: output_data_lo = Data[15:8];
        5'b??1??: output_data_lo = reg_out;
        5'b???1?: output_data_lo = bootrom_read[7:0];
        default: output_data_lo = rxbuf_read[7:0];
        endcase
    end
    always_comb begin
        casez ({psram_hi_write, output_bootrom, output_rxbuf})
        3'b1??: output_data_hi = Data[7:0];
        3'b?1?: output_data_hi = bootrom_read[15:8];
        default: output_data_hi = rxbuf_read[15:8];
        endcase
    end

    assign Data[7:0] = enable_output_lo ? output_data_lo : 8'hZZ;
    assign Data[15:8] = enable_output_hi ? output_data_hi : 8'hZZ;
    /*wire enable_output_lo_on_oe = (output_io_out | output_bootrom | output_ipcbuf | output_rxbuf);
    wire enable_output_lo_on_we = psram_hi_read;
    wire enable_output_hi = (sel_oe & (output_bootrom | output_rxbuf)) | psram_hi_write;

    // to prevent e.g. address changes from causing glitches
    // which end in enabling output for a moment
    // guard outputs with an explicit final LE where
    // the other inputs /SEL, /OE, /WE do not fluctuate
    // thus the output should be stable

    wire guarded_output_enable_1_lo, guarded_output_enable_2_lo, guarded_output_enable_hi;
    // ~I0 & ~I1 & I2
    SB_LUT4 #(.LUT_INIT(16'b0001000000010000)) output_lo_on_we_guard (
        .I0(nSel),
        .I1(nWE),
        .I3(enable_output_lo_on_we),
        .O(guarded_output_enable_1_lo));
    // (~I0 & ~I1 & I2) | I3
    SB_LUT4 #(.LUT_INIT(16'b1111111100010000)) output_lo_on_oe_guard (
        .I0(nSel),
        .I1(nOE),
        .I2(enable_output_lo_on_oe),
        .I3(guarded_output_enable_1_lo),
        .O(guarded_output_enable_2_lo));
    // ~I0 & ~I1 & I2
    SB_LUT4 #(.LUT_INIT(16'b0001000000010000)) output_hi_guard (
        .I0(nSel),
        .I1(nOE),
        .I2(enable_output_hi),
        .O(guarded_output_enable_hi));

    assign Data[7:0] = guarded_output_enable_2_lo ? output_data_lo : 8'hZZ;
    assign Data[15:8] = guarded_output_enable_hi ? output_data_hi : 8'hZZ;*/
endmodule

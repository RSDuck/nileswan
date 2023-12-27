`include "dance.sv"
`include "blockram.sv"
`include "spi_io.sv"
`include "mapper.sv"

module nileswan(
    input nSel, input nOE, input nWE,
    input nIO,

    input[7:0] AddrLo, input[3:0] AddrHi,
    inout[15:0] Data,

    output[5:0] AddrExt,
    output nPSRAMSel,
    output PSRAM_nLB_nUB,

    output MBC,
    input SClk,

    output[3:0] DebugLEDs,

    input FastClk, output FastClkEnable,
    
    output SPI_Cs,
    output SPI_Clk,
    output SPI_Do,
    input SPI_Di);

    assign FastClkEnable = 1'b1;

    Dance dance (
        .AddrLo(AddrLo),
        .AddrHi(AddrHi),
        .SClk(SClk),
        .MBC(MBC)
    );

    wire[15:0] blockram_read;
    BlockRam blockram (
        .nOE(nOE),
        .AddrLo(AddrLo),
        .ReadData(blockram_read)
    );

    reg[2:0] oe_we_fastclk = 3'h7;

    always @(posedge FastClk) begin
        oe_we_fastclk <= {oe_we_fastclk[1:0], nOE & nWE};
    end

    wire OE_WE_Rising_FastClk = ~oe_we_fastclk[2] & oe_we_fastclk[1];

    wire IOWrite = ~nSel & ~nIO;
    wire[7:0] RegAddr = {AddrHi, AddrLo[3:0]};

    reg sel_reg_spi_cnt, sel_reg_spi_data;

    wire[7:0] spi_data, spi_cnt;

    SPI_IO spi_io (
        .nWE(nWE),
        .FastClk(FastClk),
        .OE_WE_Rising_FastClk(OE_WE_Rising_FastClk),

        .WriteSPIData(sel_reg_spi_data & IOWrite),
        .WriteSPICnt(sel_reg_spi_cnt & IOWrite),
        .WriteData(Data[7:0]),

        .ReadSPIData(spi_data),
        .ReadSPICnt(spi_cnt),

        .SPI_Cs(SPI_Cs),
        .SPI_Clk(SPI_Clk),
        .SPI_Do(SPI_Do),
        .SPI_Di(SPI_Di)
    );

    reg[7:0] sram_addr_ext = 8'hFF;
    reg[7:0] rom0_addr_ext = 8'hFF;
    reg[7:0] rom1_addr_ext = 8'hFF;
    reg[7:0] rom_linear_addr_ext = 8'hFF;

    reg self_flash = 1'h0;

    reg enable_bootrom = 1'h1;

    reg[7:0] reg_out = 0;
    reg reg_ack = 0;

    //reg[3:0] debug_LEDs = 0;
    //assign DebugLEDs = debug_LEDs;
    assign DebugLEDs = sram_addr_ext[3:0];

    // Bandai chip
    localparam LINEAR_ADDR_OFF = 8'hC0;
    localparam RAM_BANK = 8'hC1;
    localparam ROM_BANK0 = 8'hC2;
    localparam ROM_BANK1 = 8'hC3;
    localparam MEMORY_CTRL = 8'hCE;

    // nileswan extension
    localparam SPI_DATA = 8'hE0;
    localparam SPI_CNT = 8'hE1;
    localparam BLKMEM_CTRL = 8'hE2;

    always_comb begin
        reg_ack = 1;
        reg_out = 0;

        sel_reg_spi_data = 0;
        sel_reg_spi_cnt = 0;

        case (RegAddr)
        LINEAR_ADDR_OFF: reg_out = rom_linear_addr_ext;
        RAM_BANK: reg_out = sram_addr_ext;
        ROM_BANK0: reg_out = rom0_addr_ext;
        ROM_BANK1: reg_out = rom1_addr_ext;
        MEMORY_CTRL: reg_out = {7'h0, self_flash};
        //8'hDF: reg_out = debug_LEDs;

        SPI_DATA: begin
            sel_reg_spi_data = 1;
            reg_out = spi_data;
        end
        SPI_CNT: begin
            sel_reg_spi_cnt = 1;
            reg_out = spi_cnt;
        end

        BLKMEM_CTRL: reg_out = {7'h0, enable_bootrom};

        default: reg_ack = 0;
        endcase
    end

    always @(posedge nWE) begin
        if (IOWrite) begin
            case (RegAddr)
            LINEAR_ADDR_OFF: rom_linear_addr_ext <= Data;
            RAM_BANK: sram_addr_ext <= Data;
            ROM_BANK0: rom0_addr_ext <= Data;
            ROM_BANK1: rom1_addr_ext <= Data;
            MEMORY_CTRL: self_flash <= Data[0];
            //8'hDF: debug_LEDs <= Data[3:0];

            BLKMEM_CTRL: enable_bootrom <= Data[0];
            default: begin end
            endcase
        end
    end

    reg sram_addr, rom0_addr, rom1_addr, rom_linear_addr;
    always_comb begin
        sram_addr = 0;
        rom0_addr = 0;
        rom1_addr = 0;
        rom_linear_addr = 0;
        case (AddrHi)
            4'h0: begin end
            4'h1: sram_addr = 1;
            4'h2: rom0_addr = 1;
            4'h3: rom1_addr = 1;
            default: rom_linear_addr = 1;
        endcase
    end

    assign AddrExt = sram_addr ? sram_addr_ext[5:0] :
                    rom0_addr ? rom0_addr_ext[5:0] :
                    rom1_addr ? rom1_addr_ext[5:0] :
                    rom_linear_addr ? {rom_linear_addr_ext[1:0], AddrHi} : 0;

    wire any_rom_addr = rom0_addr | rom1_addr | rom_linear_addr | (sram_addr & self_flash);

    wire bootrom_addr = AddrExt == 6'h3F && any_rom_addr && enable_bootrom;

    wire PSRAM_nLB_nUB = 1'h0;

    wire nPSRAMSel = ~(~nSel & nIO & any_rom_addr & ~bootrom_addr);

    /*reg[7:0] sram_upper_bits_write;
    always @(posedge nWE) begin
        if (nPSRAMSel && AddrLo[0] == 1)
            sram_upper_bits_write <= Data[7:0];
    end*/

    wire sel_oe = ~nSel & ~nOE;

    wire enable_io_out = ~nIO & reg_ack;

    wire enable_blockram = nIO & bootrom_addr;

    wire enable_output_lo = sel_oe & (enable_io_out | enable_blockram);
    wire enable_output_hi = sel_oe & enable_blockram;

    wire[7:0] output_data_lo = enable_io_out ? reg_out : blockram_read[7:0];
    wire[7:0] output_data_hi = blockram_read[15:8];

    assign Data[7:0] = enable_output_lo ? output_data_lo : 8'hZZ;
    assign Data[15:8] = enable_output_hi ? output_data_hi : 8'hZZ;
endmodule

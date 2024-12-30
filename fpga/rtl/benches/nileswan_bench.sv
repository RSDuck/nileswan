`timescale 1ns / 1ps

`include "../nileswan.sv"

module nileswan_bench ();
    `include "helper/common_signals.sv"

    initial begin
        $dumpfile("nileswan.vcd");
        $dumpvars(0, nileswan_bench);
    end

    reg nSel;

    reg[8:0] addrLo = 0;
    reg[3:0] addrHi = 0;

    reg[15:0] full_write_data = 16'hZZZZ;
    wire[15:0] read_data;
    assign read_data = full_write_data;

    wire[6:0] addrExt;

    wire nPSRAM1Sel, nPSRAM2Sel, PSRAM_nLB, PSRAM_nUB;

    wire MBC;

    wire debug;

    wire fastClkEnable;

    wire spi_cs, spi_clk, spi_do;
    reg spi_di;

    wire tf_cs, tf_clk, tf_do;
    reg tf_di;

    nileswan nswan(
        .nSel(nSel),
        .nOE(nOE),
        .nWE(nWE),
        .nIO(nIO),

        .AddrLo(addrLo),
        .AddrHi(addrHi),
        .Data(read_data),

        .AddrExt(addrExt),
        
        .nPSRAM1Sel(nPSRAM1Sel),
        .nPSRAM2Sel(nPSRAM2Sel),
        .PSRAM_nLB(PSRAM_nLB),
        .PSRAM_nUB(PSRAM_nUB),

        .MBC(MBC),
        .SClk(sclk),

        .FastClk(clk),
        .FastClkEnable(fastClkEnable),

        .nFlashSel(spi_cs),
        .SPIClk(spi_clk),
        .SPIDo(spi_do),
        .SPIDi(spi_di),

        .nTFSel(tf_cs),
        .TFClk(tf_clk),
        .TFDo(tf_do),
        .TFDi(tf_di));

    initial begin
        #(swan_clock_period*4);

        nSel = 0;
        #(swan_clock_period);

        // read period
        #(swan_clock_period/2);
        addrHi = 4'hF;
        addrLo = 8'hF0;
        #(swan_clock_period/2);
        nOE = 0;
        #(swan_clock_period);
        nOE = 1;

        #(swan_clock_period/2);
        addrHi = 4'hE;
        addrLo = 8'h01;
        #(swan_clock_period/2);
        nIO = 0;
        nWE = 0;
        full_write_data = 2;
        #(swan_clock_period);
        nWE = 1;

        #(swan_clock_period/2);
        nIO = 1;
        full_write_data = 16'hZZZZ;
        #(swan_clock_period/2);
        nIO = 0;
        nOE = 0;

        #(swan_clock_period);
        nOE = 1;
        #(swan_clock_period/2);
        nIO = 1;

        $finish;
    end
endmodule
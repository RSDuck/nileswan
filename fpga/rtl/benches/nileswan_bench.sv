`timescale 1ns / 1ps

`include "../nileswan.sv"

module nileswan_bench (
);
    // 25 MHz clock
    localparam fastclk_half_period = 1000 / 25 / 2;
    reg clk = 0;
    always #(fastclk_half_period) clk = ~clk;

    // serial clock
    localparam sclk_half_period = 1000000 / 384 / 2;
    reg sclk;
    always #(sclk_half_period) sclk = ~sclk;

    localparam swan_clock_period = 1000 / 6;

    initial begin
        $dumpfile("nileswan.vcd");
        $dumpvars(0, nileswan_bench);
    end

    reg nOE = 1, nWE = 1, nSel = 1, nIO = 1;

    reg[7:0] addrLo = 0;
    reg[3:0] addrHi = 0;

    reg[15:0] write_data = 16'hZZZZ;
    wire[15:0] read_data;
    assign read_data = write_data;

    wire[5:0] addrExt;

    wire nPSRAMSel, PSRAM_nLB_nUB;

    wire MBC;

    wire[3:0] debugLEDs;

    wire fastClkEnable;

    wire spi_cs, spi_clk, spi_do;
    reg spi_di;

    nileswan nswan(
        .nSel(nSel),
        .nOE(nOE),
        .nWE(nWE),
        .nIO(nIO),

        .AddrLo(addrLo),
        .AddrHi(addrHi),
        .Data(read_data),

        .AddrExt(addrExt),
        
        .nPSRAMSel(nPSRAMSel),
        .PSRAM_nLB_nUB(PSRAM_nLB_nUB),

        .MBC(MBC),
        .SClk(sclk),

        .DebugLEDs(debugLEDs),

        .FastClk(clk),
        .FastClkEnable(fastClkEnable),

        .SPI_Cs(spi_cs),
        .SPI_Clk(spi_clk),
        .SPI_Do(spi_do),
        .SPI_Di(spi_di));

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
        write_data = 2;
        #(swan_clock_period);
        nWE = 1;

        #(swan_clock_period/2);
        nIO = 1;
        write_data = 16'hZZZZ;
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
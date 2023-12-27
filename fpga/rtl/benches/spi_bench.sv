`timescale 1ns / 1ps

`include "../spi.sv"

module spi_bench (
);
    reg clk = 0;

    // 25 MHz clock
    always #20 clk = ~clk;

    initial begin
        $dumpfile("spi.vcd");
        $dumpvars(0, spi_bench);
    end

    reg[7:0] data_out;
    wire[7:0] data_in;

    reg keep_cs;

    wire busy;
    reg start;

    reg spi_di = 0;
    wire spi_do, spi_cs, spi_clk;

    SPI spi (
        .Clk(clk),
        .DataOut(data_out),
        .DataIn(data_in),
        .Busy(busy),
        .Start(start),

        .SPI_Cs(spi_cs),
        .SPI_Clk(spi_clk),
        .SPI_Do(spi_do),
        .SPI_Di(spi_di),

        .KeepCS(keep_cs)
    );

    reg[7:0] testdev_indata = 0;
    reg[15:0] testdev_outdata;

    always @(posedge spi_clk) begin
        if (~spi_cs)
            testdev_indata = {testdev_indata[6:0], spi_do};
    end
    always @(negedge spi_clk or negedge spi_cs) begin
        if (~spi_cs) begin
            spi_di = testdev_outdata[15];
            testdev_outdata = {testdev_outdata[14:0], 1'h0};
        end
    end

    integer i;
    initial begin
        testdev_outdata = 16'hAB13;

        keep_cs = 1;
        data_out = 8'hAE;
        start = 1;
        #40;
        start = 0;
        for (i = 0; i < 10; i++) begin
            #40;
        end
        keep_cs = 0;
        data_out = 8'hF7;
        start = 1;
        #40;
        start = 0;
        for (i = 0; i < 10; i++) begin
            #40;
        end
        $finish;
    end
endmodule
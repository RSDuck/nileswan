`timescale 1ns / 1ps

`include "../spi_io.sv"

module spi_io_bench (
);
    localparam fastclk_half_period = 1000 / 25 / 2;
    reg clk = 0;

    // 25 MHz clock
    always #(fastclk_half_period) clk = ~clk;

    initial begin
        $dumpfile("spi_io.vcd");
        $dumpvars(0, spi_io_bench);
    end

    reg nOE = 1, nWE = 1, nSel = 1, nIO = 1;

    wire IOWrite = nSel | nIO;

    wire[3:0] debug;

    reg spi_di = 0;
    wire spi_do, spi_cs, spi_clk;

    reg[2:0] oe_we_fastclk = 3'h7;

    always @(posedge clk) begin
        oe_we_fastclk <= {oe_we_fastclk[1:0], nOE & nWE};
    end

    wire OE_WE_Rising_FastClk = ~oe_we_fastclk[2] & oe_we_fastclk[1];

    reg[7:0] reg_in = 0;
    wire[7:0] spi_data, spi_cnt;

    reg sel_reg_spi_cnt = 0;
    reg sel_reg_spi_data = 0;

    SPI_IO spi (
        .FastClk(clk),
        .nWE(nWE),
        .OE_WE_Rising_FastClk(OE_WE_Rising_FastClk),

        .ReadSPIData(spi_data),
        .ReadSPICnt(spi_cnt),

        .WriteSPIData(sel_reg_spi_data & ~IOWrite),
        .WriteSPICnt(sel_reg_spi_cnt & ~IOWrite),
        .WriteData(reg_in),

        .SPI_Cs(spi_cs),
        .SPI_Clk(spi_clk),
        .SPI_Do(spi_do),
        .SPI_Di(spi_di)
    );

    reg[7:0] testdev_indata = 0;
    reg[15:0] testdev_outdata = 16'hAB13;

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
    

    localparam swan_clock_period = 1000 / 6;

    task readCycle(output[7:0] cnt, output[7:0] data);
        nIO = 0;
        nOE = 0;
        #(swan_clock_period);

        nOE = 1;
        cnt = spi_cnt;
        data = spi_data;
        #(swan_clock_period/2);
        nIO = 0;
        #(swan_clock_period/2);
    endtask

    task writeCycle(input[7:0] value, input sel_cnt, input sel_data);
        reg_in = value;
        nIO = 0;
        nWE = 0;
        sel_reg_spi_cnt = sel_cnt;
        sel_reg_spi_data = sel_data;
        #(swan_clock_period);

        nWE = 1;
        #(swan_clock_period/2);
        nIO = 1;
        #(swan_clock_period/2);
    endtask

    integer i;
    reg[7:0] read_spi_cnt, read_spi_data;
    initial begin
        #1000
        nSel = 0;
        #1000

        // read SPI_CNT
        readCycle(read_spi_cnt, read_spi_data);
        assert (read_spi_cnt == 8'h0)
        else   $error("Invalid SPI status reg %x", spi_cnt);

        writeCycle(8'h2, 1, 0);

        writeCycle(8'hD3, 0, 1);

        // query status register
        readCycle(read_spi_cnt, read_spi_data);
        assert (spi_cnt == 8'h3)
        else   $error("Invalid SPI status reg %x", spi_cnt);
        readCycle(read_spi_cnt, read_spi_data);
        assert (spi_cnt == 8'h2)
        else   $error("Invalid SPI status reg %x", spi_cnt);

        readCycle(read_spi_cnt, read_spi_data);
        assert (spi_data == 8'hAB)
        else   $error("Read invalid SPI data %x", spi_data);
        assert (testdev_indata == 8'hD3)
        else   $error("Written SPI data invalid %x", testdev_indata);

        assert (~spi_cs)
        else   $error("SPI /CS not low");

        writeCycle(8'h0, 1, 0);

        writeCycle(8'h4A, 0, 1);

        readCycle(read_spi_cnt, read_spi_data);
        readCycle(read_spi_cnt, read_spi_data);
        readCycle(read_spi_cnt, read_spi_data);

        assert (spi_data == 8'h13)
        else   $error("Read invalid SPI data %x", spi_data);
        assert (testdev_indata == 8'h4A)
        else   $error("Written SPI data invalid %x", testdev_indata);

        assert (spi_cs)
        else   $error("SPI /CS not high again");

        #1000;
        $finish;
    end
endmodule
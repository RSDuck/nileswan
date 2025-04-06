`timescale 1ns / 1ps

`include "../rtc.sv"
`include "../spimux.sv"

module cartserial_bench ();

`include "helper/common_signals.sv"
`include "helper/spi_dev.sv"

    initial begin
        $dumpfile("rtc.vcd");
        $dumpvars(0, cartserial_bench);
    end

    reg spi_di = 0;
    wire spi_cs;
    wire spi_next_do, spi_clk_enable;

    reg write_txbuffer = 0;
    reg write_spicnt_lo = 0;
    reg write_spicnt_hi = 0;

    reg sel_rtc_ctrl = 0, sel_rtc_data = 0;

    wire[7:0] serial_ctrl;
    wire[15:0] serial_com;
    wire[15:0] serial_data;

    wire[7:0] rtc_data;
    wire[7:0] rtc_ctrl;

    reg ready_falling_edge = 0;

    wire spi_reg_cs, spi_clk_running, spi_clk_stretch;
    RTC spi (
        .SClk(sclk),
        .nWE(nWE),
        .nOE(nOE),
        .WriteData(write_data),

        .MCUReadyFallingEdge(ready_falling_edge),

        .SelRTCData(sel_rtc_data),
        .SelRTCCtrl(sel_rtc_ctrl),

        .RTCCtrl(rtc_ctrl),
        .RTCData(rtc_data),

        .SPIDi(spi_di),
        .SPIDo(spi_next_do),
        .SPIClkRunning(spi_clk_running),
        .SPIClkStretch(spi_clk_stretch),
        .nMCUSel(spi_cs)
    );
    wire spi_do, spi_clk;
    SPIMux #(.SIZE(1)) spi_mux (
        .Clk(sclk),
        
        .ClockRunning(spi_clk_running),
        .ClockStretch(spi_clk_stretch),
        .InSPIDo(spi_next_do),
        .InSPISel(spi_cs),

        .OutSPIDo(spi_do),
        .OutSPIClk(spi_clk));

    task wait_spi_and_ack(input[7:0] recv_size);
        while (testdev_rx_queue.size() < recv_size) begin
            #(sclk_half_period);
        end
        #(sclk_half_period*25);
        ready_falling_edge = 1;
        #(sclk_half_period*3);
        ready_falling_edge = 0;
    endtask 

    integer i;
    `make_spi_dev(testdev, TestDev, spi_cs, spi_clk, spi_di, spi_do)

    initial begin
        #100;

        // RTC reset command
        pushTestDevTx(8'h13);
        testdev_tx_queue.push_back(0);

        sel_rtc_ctrl = 1;
        writeReg(8'h10);
        sel_rtc_ctrl = 0;

        wait_spi_and_ack(8);

        while (rtc_ctrl[4]) begin
            #(sclk_half_period);
        end
        compareTestDevRX(8'hF0);

        // RTC read status
        pushTestDevTx(8'h11);
        pushTestDevTx(8'hEE);
        testdev_tx_queue.push_back(0);

        sel_rtc_ctrl = 1;
        writeReg(8'h13);
        sel_rtc_ctrl = 0;

        wait_spi_and_ack(8);

        wait_spi_and_ack(16);

        while (rtc_ctrl[4]) begin
            #(sclk_half_period);
        end

        compareTestDevRX(8'hF3);
        // again shift register garbage
        compareTestDevRX(8'hFF);

        assert (rtc_data == 8'hEE) 
        else   $error("Wrong data in RTC data register %x", rtc_data);

        // RTC write misc
        sel_rtc_data = 1;
        writeReg(8'h22);
        sel_rtc_data = 0;

        pushTestDevTx(8'h11);
        pushTestDevTx(8'h12);
        pushTestDevTx(8'h13);
        testdev_tx_queue.push_back(0);

        sel_rtc_ctrl = 1;
        writeReg(8'h18);
        sel_rtc_ctrl = 0;

        wait_spi_and_ack(8);
        compareTestDevRX(8'hF8);

        while (~rtc_ctrl[7]) begin
            #(sclk_half_period);
        end

        compareTestDevRX(8'h22);

        sel_rtc_data = 1;
        writeReg(8'h33);
        sel_rtc_data = 0;

        wait_spi_and_ack(8);

        while (rtc_ctrl[4]) begin
            #(sclk_half_period);
        end

        compareTestDevRX(8'h33);

        // RTC read status
        pushTestDevTx(8'h11);
        pushTestDevTx(8'hAA);
        pushTestDevTx(8'hBB);
        pushTestDevTx(8'hCC);
        pushTestDevTx(8'hDD);
        pushTestDevTx(8'hEE);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'h22);
        testdev_tx_queue.push_back(0);

        sel_rtc_ctrl = 1;
        writeReg(8'h15);
        sel_rtc_ctrl = 0;

        wait_spi_and_ack(8);

        while (~rtc_ctrl[7]) begin
            #(sclk_half_period);
        end

        compareTestDevRX(8'hF5);
        compareTestDevRX(8'hFF);

        sel_rtc_data = 1;
        readReg();
        sel_rtc_data = 0;
        assert (rtc_data == 8'hAA) 
        else   $error("Wrong data in RTC data register %x", rtc_data);

        while (~rtc_ctrl[7]) begin
            #(sclk_half_period);
        end

        compareTestDevRX(8'hFF);

        sel_rtc_data = 1;
        readReg();
        sel_rtc_data = 0;
        assert (rtc_data == 8'hBB)
        else   $error("Wrong data in RTC data register %x", rtc_data);

        while (~rtc_ctrl[7]) begin
            #(sclk_half_period);
        end

        compareTestDevRX(8'hFF);

        sel_rtc_data = 1;
        readReg();
        sel_rtc_data = 0;
        assert (rtc_data == 8'hCC)
        else   $error("Wrong data in RTC data register %x", rtc_data);
        while (~rtc_ctrl[7]) begin
            #(sclk_half_period);
        end

        compareTestDevRX(8'hFF);

        sel_rtc_data = 1;
        readReg();
        sel_rtc_data = 0;
        assert (rtc_data == 8'hDD)
        else   $error("Wrong data in RTC data register %x", rtc_data);
        while (~rtc_ctrl[7]) begin
            #(sclk_half_period);
        end

        compareTestDevRX(8'hFF);

        sel_rtc_data = 1;
        readReg();
        sel_rtc_data = 0;
        assert (rtc_data == 8'hEE)
        else   $error("Wrong data in RTC data register %x", rtc_data);

        while (~rtc_ctrl[7]) begin
            #(sclk_half_period);
        end

        compareTestDevRX(8'hFF);

        sel_rtc_data = 1;
        readReg();
        sel_rtc_data = 0;
        assert (rtc_data == 8'hFF)
        else   $error("Wrong data in RTC data register %x", rtc_data);

        wait_spi_and_ack(8);

        while (~rtc_ctrl[7]) begin
            #(sclk_half_period);
        end

        compareTestDevRX(8'hFF);

        sel_rtc_data = 1;
        readReg();
        sel_rtc_data = 0;
        assert (rtc_data == 8'h22)
        else   $error("Wrong data in RTC data register %x", rtc_data);

        $finish;
    end
endmodule
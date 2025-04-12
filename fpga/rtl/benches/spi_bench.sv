`timescale 1ns / 1ps

`include "../spi.sv"
`include "../spimux.sv"
`include "../clockmux.sv"

module spi_bench ();

`include "helper/common_signals.sv"
`include "helper/spi_dev.sv"

    initial begin
        $dumpfile("spi.vcd");
        $dumpvars(0, spi_bench);
    end

    reg spi_di = 0;
    wire SPIDiInOut;
    assign SPIDiInOut = spi_di;
    wire spi_next_do, spi_do, spi_cs, spi_clk;

    reg tf_di = 0;
    wire tf_do, tf_cs, tf_clk, spi_clk_running;

    reg[9:0] buf_addr;

    wire[15:0] rx_bufdata;

    reg write_txbuffer = 0;
    reg write_spicnt_lo = 0;
    reg write_spicnt_hi = 0;
    reg write_spicnt_2 = 0;

    reg tf_pow = 0;

    wire[15:0] spi_cnt;
    wire[7:0] spi_cnt_2;

    wire transfer_clk, use_slow_clk;

    ClockMux clock_mux (
        .ClkA(clk),
        .ClkB(sclk),

        .ClkSel(use_slow_clk),

        .OutClk(transfer_clk));

    SPI spi (
        .TransferClk(transfer_clk),
        .nWE(nWE), .nOE(nOE),

        .BufAddr(buf_addr),

        .WriteData(write_data),

        .RXBufData(rx_bufdata),
        .SPICnt(spi_cnt),
        .SPICnt2(spi_cnt_2),

        .WriteTXBuffer(write_txbuffer),
        .WriteSPICntLo(write_spicnt_lo),
        .WriteSPICntHi(write_spicnt_hi),
        .WriteSPICnt2(write_spicnt_2),

        .UseSlowClk(use_slow_clk),

        .TFPow(tf_pow),

        .SPIDo(spi_next_do),
        .SPIDi(SPIDiInOut),
        .SPIClkRunning(spi_clk_running),
        .nFlashSel(spi_cs),

        .TFDo(tf_do),
        .TFDi(tf_di),
        .TFClk(tf_clk),
        .nTFSel(tf_cs)
    );

    SPIMux #(.SIZE(1)) spi_mux (
        .Clk(transfer_clk),
        
        .ClockRunning(spi_clk_running),
        .ClockStretch(1'b0),
        .InSPIDo(spi_next_do),
        .InSPISel(spi_cs),

        .OutSPIDo(spi_do),
        .OutSPIClk(spi_clk));

    integer i;

    `make_spi_dev(testdev, TestDev, spi_cs, spi_clk, spi_di, spi_do)
    `make_spi_dev(testtf, TestTF, tf_cs, tf_clk, tf_di, tf_do)

    task readRXBuf(input[9:0] addr, output[15:0] data);
        buf_addr = addr;
        #(swan_clock_period/2);
        nOE = 0;
        #(swan_clock_period);
        nOE = 1;
        data = rx_bufdata;
        #(swan_clock_period/2);
    endtask

    task writeTXBuf(input[9:0] addr, input[7:0] data);
        buf_addr = addr;
        write_txbuffer = 1;
        #(swan_clock_period/2);
        write_data = data;
        nWE = 0;
        #(swan_clock_period);
        nWE = 1;
        #(swan_clock_period/2);
        write_txbuffer = 0;
    endtask

    localparam SPI_CNT_START = 8'h1 << (15-8);
    localparam SPI_CNT_BUFFER = 8'h1 << (14-8);
    localparam SPI_CNT_EXCHANGE = 8'h2 << (12-8);
    localparam SPI_CNT_WAIT_AND_READ = 8'h3 << (12-8);

    localparam SPI_CNT2_SLOW_CLK = 8'h01;
    localparam SPI_CNT2_DEV_FLASH = 8'h2 << 1;

    reg[15:0] read_spi_data;
    initial begin
        #1000

        // exchange
        writeTXBuf(0, 8'hAB);
        writeTXBuf(1, 8'hCD);
        writeTXBuf(2, 8'hEF);
        writeTXBuf(3, 8'h12);

        pushTestDevTx(8'hFF);
        pushTestDevTx(8'h3E);
        pushTestDevTx(8'hCA);
        pushTestDevTx(8'h04);
        // resting value to prevent warning
        testdev_tx_queue.push_back(1);

        write_spicnt_lo = 1;
        writeReg(3);
        write_spicnt_lo = 0;

        assert (spi_cs == 1) 
        else   $error("/CS should be high");

        write_spicnt_2 = 1;
        writeReg(SPI_CNT2_DEV_FLASH | SPI_CNT2_SLOW_CLK);
        write_spicnt_2 = 0;

        write_spicnt_hi = 1;
        writeReg(SPI_CNT_START | SPI_CNT_EXCHANGE | SPI_CNT_BUFFER);
        write_spicnt_hi = 0;

        assert (spi_cnt_2[0] == 1) 
        else   $error("slow clock bit should be 1");

        assert (spi_cs == 0) 
        else   $error("/CS should be low now");

        while (spi_cnt[15]) begin
            readRXBuf(0, read_spi_data);
        end

        // swap buffers back and deassert /CS
        write_spicnt_hi = 1;
        writeReg(8'h0);
        write_spicnt_hi = 0;

        write_spicnt_2 = 1;
        writeReg(8'h0);
        write_spicnt_2 = 0;

        assert (spi_cs == 1) 
        else   $error("/CS is not high again");

        readRXBuf(0, read_spi_data);
        assert (read_spi_data == 16'h3EFF)
        else   $error("Wrong value read from SPI RX buffer (offset=0) got %x", read_spi_data);
        readRXBuf(2, read_spi_data);
        assert (read_spi_data == 16'h04CA)
        else   $error("Wrong value read from SPI RX buffer (offset=2) got %x", read_spi_data);
        readRXBuf(4, read_spi_data);
        assert (read_spi_data == 16'h0000)
        else   $error("Wrong value read from SPI RX buffer (offset=4) got %x", read_spi_data);

        compareTestDevRX(8'hAB);
        compareTestDevRX(8'hCD);
        compareTestDevRX(8'hEF);
        compareTestDevRX(8'h12);

        // multiple bytes wait and read
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'h53);
        pushTestDevTx(8'h85);
        pushTestDevTx(8'hF0);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'h21);
        // resting value to prevent warning
        testdev_tx_queue.push_back(1);

        write_spicnt_2 = 1;
        writeReg(SPI_CNT2_DEV_FLASH);
        write_spicnt_2 = 0;

        write_spicnt_lo = 1;
        writeReg(4);
        write_spicnt_lo = 0;

        write_spicnt_hi = 1;
        writeReg(SPI_CNT_START | SPI_CNT_WAIT_AND_READ | SPI_CNT_BUFFER);
        write_spicnt_hi = 0;

        assert (spi_cnt_2[0] == 0)
        else   $error("slow clock bit should be 1");

        assert (spi_cs == 0) 
        else   $error("/CS should be low now");

        while (spi_cnt[15]) begin
            readRXBuf(0, read_spi_data);
        end

        // swap buffers back and deassert /CS
        write_spicnt_hi = 1;
        writeReg(8'h0);
        write_spicnt_hi = 0;

        write_spicnt_2 = 1;
        writeReg(8'h0);
        write_spicnt_2 = 0;

        assert (spi_cs == 1) 
        else   $error("/CS is not high again");

        readRXBuf(0, read_spi_data);
        assert (read_spi_data == 16'h8553)
        else   $error("Wrong value read from SPI RX buffer (offset=0) got %x", read_spi_data);
        readRXBuf(2, read_spi_data);
        assert (read_spi_data == 16'hFFF0)
        else   $error("Wrong value read from SPI RX buffer (offset=2) got %x", read_spi_data);
        readRXBuf(4, read_spi_data);
        assert (read_spi_data == 16'h0021)
        else   $error("Wrong value read from SPI RX buffer (offset=4) got %x", read_spi_data);

        compareTestDevRX(8'hFF);
        compareTestDevRX(8'hFF);
        compareTestDevRX(8'hFF);
        compareTestDevRX(8'hFF);
        compareTestDevRX(8'hFF);
        compareTestDevRX(8'hFF);
        compareTestDevRX(8'hFF);
        compareTestDevRX(8'hFF);

        // aborted wait and read
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        testdev_tx_queue.push_back(1);

        write_spicnt_2 = 1;
        writeReg(SPI_CNT2_DEV_FLASH);
        write_spicnt_2 = 0;

        write_spicnt_lo = 1;
        writeReg(0);
        write_spicnt_lo = 0;

        write_spicnt_hi = 1;
        writeReg(SPI_CNT_START | SPI_CNT_BUFFER | SPI_CNT_WAIT_AND_READ);
        write_spicnt_hi = 0;

        #(fastclk_half_period*20);

        write_spicnt_hi = 1;
        writeReg(8'h0);
        write_spicnt_hi = 0;

        while (spi_cnt[15]) begin
            readRXBuf(0, read_spi_data);
        end

        testdev_tx_queue.delete();
        testdev_rx_queue.delete();

        // swap buffers back and deassert /CS
        write_spicnt_hi = 1;
        writeReg(8'h0);
        write_spicnt_hi = 0;

        write_spicnt_2 = 1;
        writeReg(0);
        write_spicnt_2 = 0;

        // wait and read with a single byte
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hE3);
        // resting value to prevent warning
        testdev_tx_queue.push_back(1);

        write_spicnt_2 = 1;
        writeReg(SPI_CNT2_DEV_FLASH | SPI_CNT2_SLOW_CLK);
        write_spicnt_2 = 0;

        write_spicnt_lo = 1;
        writeReg(0);
        write_spicnt_lo = 0;

        write_spicnt_hi = 1;
        writeReg(SPI_CNT_START | SPI_CNT_WAIT_AND_READ | SPI_CNT_BUFFER);
        write_spicnt_hi = 0;

        assert (spi_cnt_2[0] == 1) 
        else   $error("slow clock bit should be 1");

        assert (spi_cs == 0) 
        else   $error("/CS should be low now");

        while (spi_cnt[15]) begin
            readRXBuf(0, read_spi_data);
        end

        // swap buffers back and deassert /CS
        write_spicnt_hi = 1;
        writeReg(8'h0);
        write_spicnt_hi = 0;

        write_spicnt_2 = 1;
        writeReg(8'h0);
        write_spicnt_2 = 0;

        assert (spi_cs == 1) 
        else   $error("/CS is not high again");

        readRXBuf(0, read_spi_data);
        assert (read_spi_data == 16'h85E3) // 85 is from the previous transfer
        else   $error("Wrong value read from SPI RX buffer (offset=0) got %x", read_spi_data);

        compareTestDevRX(8'hFF);
        compareTestDevRX(8'hFF);
        compareTestDevRX(8'hFF);
        compareTestDevRX(8'hFF);
        compareTestDevRX(8'hFF);
        compareTestDevRX(8'hFF);
        compareTestDevRX(8'hFF);
/*
        tf_pow = 1;

        writeTXBuf(0, 8'h63);
        writeTXBuf(1, 8'h82);
        writeTXBuf(2, 8'hB3);
        writeTXBuf(3, 8'h99);

        write_spicnt_lo = 1;
        writeReg(3);
        write_spicnt_lo = 0;

        write_spicnt_hi = 1;
        // start transfer, mode=write, device=tf, /cs = lo, use other buffer
        writeReg(8'h80 | (8'h0 << 1) | (8'h1 << 4) | (8'h1 << 5) | (8'h1 << 6));
        write_spicnt_hi = 0;

        assert (spi_cs == 1) 
        else   $error("SPI /CS should not be low");
        assert (tf_cs == 0) 
        else   $error("TF /CS should be low");

        while (spi_cnt[15]) begin
            readRXBuf(0, read_spi_data);
        end

        compareTestDevRX(8'h63);
        compareTestDevRX(8'h82);
        compareTestDevRX(8'hB3);
        compareTestDevRX(8'h99);*/

        $finish;
    end
endmodule
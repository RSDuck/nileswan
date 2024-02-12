`timescale 1ns / 1ps

`include "../spi.sv"

module spi_bench ();

`include "helper/common_signals.sv"

    initial begin
        $dumpfile("spi.vcd");
        $dumpvars(0, spi_bench);
    end

    reg nOE = 1, nWE = 1, nIO = 1;

    reg spi_di = 0;
    wire spi_do, spi_cs, spi_clk;

    reg tf_di = 0;
    wire tf_do, tf_cs, tf_clk;

    reg[8:0] buf_addr;
    reg[7:0] write_data;

    wire[15:0] rx_bufdata;

    reg write_txbuffer = 0;
    reg write_spicnt_lo = 0;
    reg write_spicnt_hi = 0;

    reg tf_pow = 0;

    wire[15:0] spi_cnt;

    SPI spi (
        .FastClk(clk),
        .SClk(sclk),
        .nWE(nWE), .nOE(nOE),

        .BufAddr(buf_addr),

        .WriteData(write_data),

        .RXBufData(rx_bufdata),
        .SPICnt(spi_cnt),

        .WriteTXBuffer(write_txbuffer),
        .WriteSPICntLo(write_spicnt_lo),
        .WriteSPICntHi(write_spicnt_hi),

        .TF_Pow(tf_pow),

        .SPI_Do(spi_do),
        .SPI_Di(spi_di),
        .SPI_Clk(spi_clk),
        .SPI_Cs(spi_cs),

        .TF_Do(tf_do),
        .TF_Di(tf_di),
        .TF_Clk(tf_clk),
        .TF_Cs(tf_cs)
    );

    integer i;

    bit testdev_rx_queue[$];
    bit testdev_tx_queue[$];

    bit testtf_rx_queue[$];
    bit testtf_tx_queue[$];

    always @(posedge spi_clk) begin
        if (~spi_cs)
            testdev_rx_queue.push_back(spi_do);
    end
    always @(negedge spi_clk or negedge spi_cs) begin
        if (~spi_cs) begin
            spi_di = testdev_tx_queue.pop_front();
        end
    end

    always @(posedge tf_clk) begin
        if (~tf_cs)
            testtf_rx_queue.push_back(tf_do);
    end
    always @(negedge tf_clk or negedge tf_cs) begin
        if (~tf_cs) begin
            tf_di = testtf_tx_queue.pop_front();
        end
    end

    localparam swan_clock_period = 1000 / 6;

    task pushTestDevTx(input[7:0] data);
        for (i = 7; i >= 0; i--) begin
            testdev_tx_queue.push_back(data[i]);
        end
    endtask

    task compareTestDevRX(input[7:0] should);
        reg[7:0] got;
        for (i = 7; i >= 0; i--) begin
            got[i] = testdev_rx_queue.pop_front();
        end
        assert (got == should) 
        else   $error("SPI test device data mismatch (expected %x got %x)", should, got);
    endtask

    task readRXBuf(input[8:0] addr, output[15:0] data);
        buf_addr = addr;
        #(swan_clock_period/2);
        nOE = 0;
        #(swan_clock_period);
        nOE = 1;
        data = rx_bufdata;
        #(swan_clock_period/2);
    endtask

    task writeTXBuf(input[8:0] addr, input[7:0] data);
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

    task writeReg(input[7:0] data);
        write_data = data;
        #(swan_clock_period/2);
        nIO = 0;
        nWE = 0;
        #(swan_clock_period);
        nWE = 1;
        #(swan_clock_period/2);
        nIO = 1;
    endtask

    reg[15:0] read_spi_data;
    initial begin
        #1000

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

        write_spicnt_hi = 1;
        // start transfer, slowclk, mode=exchange, /cs = lo, use other buffer
        writeReg(8'h80 | (8'h2 << 1) | (8'h1 << 3) | (8'h1 << 4) | (8'h1 << 6));
        write_spicnt_hi = 0;

        assert (spi_cs == 0) 
        else   $error("/CS should be low now");

        while (spi_cnt[15]) begin
            readRXBuf(0, read_spi_data);
        end

        // swap buffers back and deassert /CS
        write_spicnt_hi = 1;
        writeReg(8'h0);
        write_spicnt_hi = 0;

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

        write_spicnt_lo = 1;
        writeReg(4);
        write_spicnt_lo = 0;

        write_spicnt_hi = 1;
        // start transfer, mode=wait and read, /cs = lo, use other buffer
        writeReg(8'h80 | (8'h3 << 1) | (8'h1 << 4) | (8'h1 << 6));
        write_spicnt_hi = 0;

        assert (spi_cs == 0) 
        else   $error("/CS should be low now");

        while (spi_cnt[15]) begin
            readRXBuf(0, read_spi_data);
        end

        // swap buffers back and deassert /CS
        write_spicnt_hi = 1;
        writeReg(8'h0);
        write_spicnt_hi = 0;

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

        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hFF);
        pushTestDevTx(8'hE3);
        // resting value to prevent warning
        testdev_tx_queue.push_back(1);

        write_spicnt_lo = 1;
        writeReg(0);
        write_spicnt_lo = 0;

        write_spicnt_hi = 1;
        // start transfer, mode=wait and read, /cs = lo, use other buffer
        writeReg(8'h80 | (8'h3 << 1) | (8'h1 << 4) | (8'h1 << 6));
        write_spicnt_hi = 0;

        assert (spi_cs == 0) 
        else   $error("/CS should be low now");

        while (spi_cnt[15]) begin
            readRXBuf(0, read_spi_data);
        end

        // swap buffers back and deassert /CS
        write_spicnt_hi = 1;
        writeReg(8'h0);
        write_spicnt_hi = 0;

        assert (spi_cs == 1) 
        else   $error("/CS is not high again");

        readRXBuf(0, read_spi_data);
        assert (read_spi_data == 16'h85E3) // 85 is from the previous transfer
        else   $error("Wrong value read from SPI RX buffer (offset=0) got %x", read_spi_data);
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
`timescale 1ns / 1ps

`include "../eeprom.sv"
`include "../spimux.sv"

module eeprom_bench ();

`include "helper/common_signals.sv"
`include "helper/spi_dev.sv"

    initial begin
        $dumpfile("eeprom.vcd");
        $dumpvars(0, eeprom_bench);
    end

    reg sel_serial_ctrl = 0;
    reg sel_serial_com_lo = 0, sel_serial_com_hi = 0;
    reg sel_serial_data_lo = 0, sel_serial_data_hi = 0;

    wire[7:0] serial_ctrl;
    wire[15:0] serial_com, serial_data;

    wire spi_sel, spi_next_do, spi_clk_running;

    reg ready_falling_edge = 1'b0;

    EEPROM eeprom (
        .SClk(sclk),
        .nWE(nWE),
        .nOE(nOE),

        .MCUReadyFallingEdge(ready_falling_edge),

        .WriteData(write_data),

        .EEPROMSize(2'b0),

        .SelSerialCtrl(sel_serial_ctrl),
        .SelSerialComLo(sel_serial_com_lo),
        .SelSerialComHi(sel_serial_com_hi),
        .SelSerialDataLo(sel_serial_data_lo),
        .SelSerialDataHi(sel_serial_data_hi),

        .SerialCtrl(serial_ctrl),
        .SerialCom(serial_com),
        .SerialData(serial_data),

        .SPISel(spi_sel),
        .SPIDo(spi_next_do),
        .SPIClkRunning(spi_clk_running)
    );

    wire spi_do, spi_clk;
    SPIMux #(.SIZE(1)) spi_mux (
        .Clk(sclk),
        
        .ClockRunning(spi_clk_running),
        .ClockStretch(1'b0),
        .InSPIDo(spi_next_do),
        .InSPISel(spi_sel),

        .OutSPIDo(spi_do),
        .OutSPIClk(spi_clk));

    integer i;
    reg spi_din;
    `make_spi_dev(testdev, TestDev, spi_sel, spi_clk, spi_din, spi_do)

    localparam EEPROM_ADDR_SIZE = 6;
    localparam COM_START = 16'h1 << (EEPROM_ADDR_SIZE+2);
    localparam COM_EXTENDED = 16'h0 << EEPROM_ADDR_SIZE;
    localparam COM_WRITE = 16'h1 << EEPROM_ADDR_SIZE;
    localparam COM_READ = 16'h2 << EEPROM_ADDR_SIZE;
    localparam COM_ERASE = 16'h3 << EEPROM_ADDR_SIZE;
    localparam COM_ERASEALL = 16'h2 << (EEPROM_ADDR_SIZE-2);
    localparam COM_WRITEALL = 16'h1 << (EEPROM_ADDR_SIZE-2);
    localparam COM_EWDS = 16'h0 << (EEPROM_ADDR_SIZE-2);
    localparam COM_EWEN = 16'h3 << (EEPROM_ADDR_SIZE-2);

    localparam CTRL_ERASE = 8'h1 << 6;
    localparam CTRL_WRITE = 8'h1 << 5;
    localparam CTRL_READ = 8'h1 << 4;

    task doErase(input[15:0] param, input check_spi);
        sel_serial_com_lo = 1;
        writeReg(COM_START|param);
        sel_serial_com_lo = 0;

        sel_serial_com_hi = 1;
        writeReg((COM_START|param)>>8);
        sel_serial_com_hi = 0;

        if (check_spi) begin
            pushTestDevTx(8'hFF);
            pushTestDevTx(8'hFF);
            testdev_tx_queue.push_back(1'b1);
            //$display("erase start %x", COM_START|param);
        end

        sel_serial_ctrl = 1;
        writeReg(CTRL_ERASE);
        sel_serial_ctrl = 0;

        if (check_spi) begin
            while (testdev_rx_queue.size() < 16) begin
                #(sclk_half_period);
            end
            #(sclk_half_period*10);

            ready_falling_edge = 1;
            #(sclk_half_period*3);
            ready_falling_edge = 0;

        end


        while (~serial_ctrl[1]) begin
            #(sclk_half_period);
        end

        if (check_spi) begin
            //$display("erase end %d", testdev_tx_queue.size());
            //$display("erase end %d", testdev_rx_queue.size());
            compareTestDevRX((COM_START|param)>>8);
            compareTestDevRX(COM_START|param);
        end
    endtask

    task doWrite(input[9:0] addr, input[15:0] value, input[15:0] param, input check_spi);
        sel_serial_com_lo = 1;
        writeReg(COM_START|param|addr);
        sel_serial_com_lo = 0;

        sel_serial_com_hi = 1;
        writeReg((COM_START|param|addr)>>8);
        sel_serial_com_hi = 0;

        sel_serial_data_lo = 1;
        writeReg(value);
        sel_serial_data_lo = 0;

        sel_serial_data_hi = 1;
        writeReg(value>>8);
        sel_serial_data_hi = 0;

        if (check_spi) begin
            pushTestDevTx(8'hFF);
            pushTestDevTx(8'hFF);
            pushTestDevTx(8'hFF);
            pushTestDevTx(8'hFF);
            testdev_tx_queue.push_back(1'b1);
            //$display("write start");
        end

        sel_serial_ctrl = 1;
        writeReg(CTRL_WRITE);
        sel_serial_ctrl = 0;

        assert (~serial_ctrl[1])
        else   $error("Busy bit was not raised?");

        if (check_spi) begin
            while (testdev_rx_queue.size() < 32) begin
                #(sclk_half_period);
            end

            #(sclk_half_period*10);

            ready_falling_edge = 1;
            #(sclk_half_period*3);
            ready_falling_edge = 0;
        end

        while (~serial_ctrl[1]) begin
            #(sclk_half_period);
        end

        if (check_spi) begin
            //$display("write end start");
            compareTestDevRX((COM_START|param|addr)>>8);
            compareTestDevRX(COM_START|param|addr);
            compareTestDevRX(value>>8);
            compareTestDevRX(value);
        end
    endtask

    task doRead(input[9:0] addr, output[15:0] value);
        sel_serial_com_lo = 1;
        writeReg(COM_START|COM_READ|addr);
        sel_serial_com_lo = 0;

        sel_serial_com_hi = 1;
        writeReg((COM_START|COM_READ|addr)>>8);
        sel_serial_com_hi = 0;

        sel_serial_ctrl = 1;
        writeReg(CTRL_READ);
        sel_serial_ctrl = 0;

        #(sclk_half_period);

        value = serial_data;
    endtask

    reg[15:0] read_data;
    initial begin
        doErase(COM_EXTENDED|COM_EWEN, 0);

        doErase(COM_EXTENDED|COM_ERASEALL, 1);

        // read out third word
        doRead(3, read_data);
        assert (read_data == 16'hFFFF)
        else   $error("Erase did not work? Got %x", read_data);

        // write single word to third word
        doWrite(3, 16'hABBA, COM_WRITE, 1);

        // read out third word
        doRead(3, read_data);
        assert (read_data == 16'hABBA)
        else   $error("Read %x, expected 0xABBA", read_data);

        // test edge cases of write all
        doWrite(63, 16'h1234, COM_WRITE, 1);
        doWrite(0, 16'h7001, COM_WRITE, 1);

        doRead(63, read_data);
        assert (read_data == 16'h1234)
        else   $error("Highest addr not written (got %x)", read_data);
        doRead(0, read_data);
        assert (read_data == 16'h7001)
        else   $error("Lowest addr not written (got %x)", read_data);

        doWrite(0, 16'h3333, COM_EXTENDED|COM_WRITEALL, 1);

        doRead(63, read_data);
        assert (read_data == 16'h3333)
        else   $error("Highest addr not overwritten (got %x)", read_data);

        doRead(0, read_data);
        assert (read_data == 16'h3333)
        else   $error("Lowest addr not overwritten (got %x)", read_data);

        // test write enable/disable
        doErase(COM_EXTENDED|COM_EWDS, 0);

        doErase(COM_ERASE, 0);

        doRead(0, read_data);
        assert (read_data == 16'h3333)
        else   $error("Memory was not protected (got %x)", read_data);

        doErase(COM_EXTENDED|COM_EWEN, 0);
        doErase(COM_ERASE, 1);
        doRead(0, read_data);
        assert (read_data == 16'hFFFF)
        else   $error("Single erase did not work (got %x)", read_data);

        $finish;
    end

endmodule
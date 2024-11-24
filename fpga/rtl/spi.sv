`include "clockmux.sv"
`include "blockram_16rn_8w.sv"
`include "blockram_8r_8w.sv"

module SPI (
    input FastClk,
    input SClk,
    input nWE, input nOE,

    input[8:0] BufAddr,

    input[7:0] WriteData,

    output[15:0] RXBufData,
    output[15:0] SPICnt,

    input WriteTXBuffer,
    input WriteSPICntLo,
    input WriteSPICntHi,

    output SPIDo,
    inout SPIDi,
    output SPIClk,
    output nFlashSel,
    output nMCUSel,

    input TFPow,

    output nTFSel,
    output TFClk,
    output TFDo,
    input TFDi);

    reg[2:0] start_transfer_clk = 3'h0;
    reg start_async = 1'h0;
    wire Start = start_transfer_clk[2] ^ start_transfer_clk[1];

    reg feedback_transfer_clk = 1'h0;
    reg[1:0] feedback_async = 2'h0;
    wire Busy = feedback_async[1] ^ start_async;

    reg[8:0] byte_position;
    reg[2:0] bit_position = 0;
    wire[8:0] NextBytePosition = byte_position + 1;
    reg[8:0] transfer_len;

    reg[7:0] shiftreg;

    reg running = 0;

    typedef enum reg[1:0] {
        channelCS_DeselTF,
        channelCS_SelTF,
        channelCS_SelFlash,
        channelCS_SelMCU
    } ChannelCS;
    ChannelCS channel_cs = channelCS_DeselTF;

    assign nFlashSel = ~(channel_cs == channelCS_SelFlash);
    assign nMCUSel = ~(channel_cs == channelCS_SelMCU);
    assign nTFSel = TFPow ? ~(channel_cs == channelCS_SelTF) : 1'bZ;

    assign SPIDi = channel_cs == channelCS_DeselTF ? 1'b1 : 1'bZ;

    // the first will be the last and the last will be the first
    wire[7:0] ShiftRegNext = {shiftreg[6:0], (channel_cs == channelCS_SelFlash ||
        channel_cs == channelCS_SelMCU) ? SPIDi : TFDi};

    reg use_slow_clk = 0;

    wire transfer_clk;
    ClockMux clk_mux (
        .ClkA(FastClk),
        .ClkB(SClk),
        .ClkSel(use_slow_clk),
        .OutClk(transfer_clk)
    );

    typedef enum reg[1:0] {
        mode_Write,
        mode_Read,
        mode_Exchange,
        mode_WaitAndRead
    } TransferMode;
    TransferMode mode = mode_Write;

    reg bus_mapped_buffer = 0;

    wire ShiftRegHasZeroBit = ShiftRegNext != 8'hFF;

    reg seen_non_filler = 0;

    wire StoreShiftReg = bit_position == 7 && mode != mode_Write;

    wire[15:0] rx_read;
    BlockRAM16RN_8W rx_buffer0 (
        .ReadClk(nOE),
        .ReadEnable(1'b1),
        .ReadAddr({~bus_mapped_buffer, BufAddr[8:1]}),
        .ReadData(rx_read),

        .WriteClk(transfer_clk),
        .WriteEnable(StoreShiftReg),
        .WriteAddr({bus_mapped_buffer, byte_position}),
        .WriteData(ShiftRegNext)
    );
    assign RXBufData = rx_read;

    wire[7:0] tx_read;
    BlockRAM8R_8W tx_buffer (
        .ReadClk(transfer_clk),
        .ReadEnable(1'b1),
        .ReadAddr({~bus_mapped_buffer, NextBytePosition}),
        .ReadData(tx_read),

        .WriteClk(nWE),
        .WriteEnable(WriteTXBuffer),
        .WriteAddr({bus_mapped_buffer, BufAddr}),
        .WriteData(WriteData)
    );

    always @(posedge transfer_clk)
        start_transfer_clk <= {start_transfer_clk[1:0], start_async};

    wire FillerOver = (seen_non_filler || ShiftRegHasZeroBit);
    wire LastPeriod = (byte_position == transfer_len) && bit_position == 7 && FillerOver;

    always @(posedge transfer_clk) begin
        if (Start)
            running <= 1;
        else if (LastPeriod)
            running <= 0;
    end

    reg last_period_delay;

    always @(posedge transfer_clk) begin
        if (LastPeriod)
            last_period_delay <= 1;
        if (last_period_delay) begin
            feedback_transfer_clk <= start_transfer_clk[2];
            last_period_delay <= 0;
        end
    end

    always @(posedge transfer_clk) begin
        if (Start)
            seen_non_filler <= mode != mode_WaitAndRead;
        else if (bit_position == 7 && ShiftRegHasZeroBit)
            seen_non_filler <= 1;
    end

    always @(posedge transfer_clk) begin
        if (Start || (bit_position == 7 && FillerOver))
            byte_position <= NextBytePosition;
        else if (~running) // reset position so that the TX buffer will read
            byte_position <= 9'h1FF;
    end

    always @(posedge transfer_clk) begin
        if (Start || running)
            shiftreg <= (bit_position == 7 || Start) ? tx_read : ShiftRegNext;
    end

    always @(posedge transfer_clk) begin
        if (running)
            bit_position <= bit_position + 1;
    end

    SB_IO #(
        .PIN_TYPE(6'b010001), // PIN_OUTPUT_DDR
        .IO_STANDARD("SB_LVCMOS")
    ) spi_clk_iob (
        .PACKAGE_PIN(SPIClk),
        .D_OUT_0(running),
        .D_OUT_1(1'b0),
        .OUTPUT_CLK(transfer_clk),
        .OUTPUT_ENABLE(1'b1),
        .CLOCK_ENABLE(1'b1)
    );
    SB_IO #(
        .PIN_TYPE(6'b100000), // PIN_OUTPUT_DDR_ENABLE
        .IO_STANDARD("SB_LVCMOS")
    ) tf_clk_iob (
        .PACKAGE_PIN(TFClk),
        .D_OUT_0(running),
        .D_OUT_1(1'b0),
        .OUTPUT_CLK(transfer_clk),
        .OUTPUT_ENABLE(TFPow),
        .CLOCK_ENABLE(1'b1)
    );

    /*
        Note that DDR buffers don't latch both output values
        on the positive edge. Instead they latch each value
        at the edge it should be output.

        This means a flip flop set on the positive edge
        (like our shift register here) will already be output
        on the next negative edge (so within the same period!).

        On the next positive edge the same value will be output again.
        (because the new value is just being latched into shiftreg,
            not into the output buffer yet)

        This whole phenomenom makes the DDR output buffers
        slightly counter intuitive, because D_OUT_1 will be output
        before D_OUT_0 assuming the values going to both flip flops
        are both latched on the rising edge.
    */

    wire outbit = (mode == mode_Write || mode == mode_Exchange) ? shiftreg[7] : 1;

    SB_IO #(
        .PIN_TYPE(6'b010001), // PIN_OUTPUT_DDR
        .IO_STANDARD("SB_LVCMOS")
    ) spi_do_iob (
        .PACKAGE_PIN(SPIDo),
        .D_OUT_0(outbit),
        .D_OUT_1(outbit),
        .OUTPUT_CLK(transfer_clk),
        .OUTPUT_ENABLE(1'b1),
        .CLOCK_ENABLE(1'b1)
    );
    SB_IO #(
        .PIN_TYPE(6'b100000), // PIN_OUTPUT_DDR_ENABLE
        .IO_STANDARD("SB_LVCMOS")
    ) tf_do_iob (
        .PACKAGE_PIN(TFDo),
        .D_OUT_0(outbit),
        .D_OUT_1(outbit),
        .OUTPUT_CLK(transfer_clk),
        .OUTPUT_ENABLE(TFPow),
        .CLOCK_ENABLE(1'b1)
    );

    assign SPICnt = {Busy, bus_mapped_buffer, channel_cs, use_slow_clk, mode, transfer_len};

    // technically this means that just waiting until the transfer is done
    // without polling is not possible, because the busy state will block writes to SPI_CNT
    always @(negedge nOE)
        feedback_async <= {feedback_async[0], feedback_transfer_clk};

    always @(posedge nWE) begin
        if (WriteSPICntLo & ~Busy)
            transfer_len[7:0] <= WriteData;
    end

    always @(posedge nWE) begin
        if (WriteSPICntHi) begin
            if (~Busy) begin
                transfer_len[8] <= WriteData[0];

                mode <= WriteData[2:1];
                // since the switchover takes three cycles
                // when starting a transfer and switching the clock at the same time
                // the cycle where Start=1 will be completed will be the "weird"
                // switch over cycle with a lengthened period
                // though it doesn't affect the outgoing SPI transfer
                use_slow_clk <= WriteData[3];
                channel_cs <= WriteData[5:4];

                bus_mapped_buffer <= WriteData[6];

                if (WriteData[7])
                    start_async <= start_async ^ 1;
            end else if (~WriteData[7]) begin
                // abort transfer
            end
        end
    end
endmodule
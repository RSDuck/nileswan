module RTC(
    input SClk,

    input nOE,
    input nWE,

    input[7:0] WriteData,

    input SelRTCData,
    input SelRTCCtrl,

    output[7:0] RTCData,
    output[7:0] RTCCtrl,

    input SPIDi,
    output SPIDo,
    output nMCUSel,

    output SPIClkRunning,
    output SPIClkStretch
);
    typedef enum reg[3:0] {
        cmd_RTCResetW,
        cmd_RTCResetR,
        cmd_RTCStatusW,
        cmd_RTCStatusR,
        cmd_RTCData0W,
        cmd_RTCData0R,
        cmd_RTCData4W,
        cmd_RTCData4R,
        cmd_RTCMiscRegW,
        cmd_RTCMiscRegR,
        cmd_RTCNopW,
        cmd_RTCNopR,
        cmd_RTCInvalid6W,
        cmd_RTCInvalid6R,
        cmd_RTCInvalid7W,
        cmd_RTCInvalid7R
    } Command;
    Command cmd;

    reg[1:0] start_cmd = 2'h0;

    reg[7:0] data_written = 8'h0;
    reg[7:0] data_recv = 8'h0;

    reg data_src_nWE = 0;
    reg data_src_RX = 0;
    wire UseWrittenData = data_src_nWE ^ data_src_RX;

    reg ready_bit_sclk = 0;
    reg ready_bit_nOE = 0, ready_bit_nWE = 0;

    wire Ready = ready_bit_sclk ^ ready_bit_nOE ^ ready_bit_nWE;

    typedef enum reg[6:0] {
        state_Idle,
        state_LowerCS,
        state_RaiseCS,
        state_Byte0Bit0 = 8,
        state_Byte1Bit0 = state_Byte0Bit0+8,
        state_WaitData1 = state_Byte1Bit0+8,
        state_Byte2Bit0 = state_WaitData1+1,
        state_WaitData2 = state_Byte2Bit0+8,
        state_Byte3Bit0 = state_WaitData2+1,
        state_WaitData3 = state_Byte3Bit0+8,
        state_Byte4Bit0 = state_WaitData3+1,
        state_WaitData4 = state_Byte4Bit0+8,
        state_Byte5Bit0 = state_WaitData4+1,
        state_WaitData5 = state_Byte5Bit0+8,
        state_Byte6Bit0 = state_WaitData5+1,
        state_WaitData6 = state_Byte6Bit0+8,
        state_Byte7Bit0 = state_WaitData6+1
    } SPIState;
    SPIState spi_state = state_Idle;
    reg[7:0] shiftreg = 8'h0;

    wire Shifting = spi_state[6:3] != 0;

    wire StartRTCCmd = start_cmd[1] ^ start_cmd[0];

    reg[6:0] cmd_final_state;
    always_comb begin
        case (cmd)
        cmd_RTCResetW,
        cmd_RTCResetR: cmd_final_state = state_Byte0Bit0+7;
        cmd_RTCStatusW,
        cmd_RTCStatusR: cmd_final_state = state_Byte1Bit0+7;
        cmd_RTCData0W,
        cmd_RTCData0R: cmd_final_state = state_Byte7Bit0+7;
        cmd_RTCData4W,
        cmd_RTCData4R: cmd_final_state = state_Byte3Bit0+7;
        cmd_RTCNopW,
        cmd_RTCNopR,
        cmd_RTCMiscRegW,
        cmd_RTCMiscRegR: cmd_final_state = state_Byte2Bit0+7;
        default: cmd_final_state = 6'h00;
        endcase
    end

    always @(posedge SClk) begin
        case (spi_state)
        state_Idle: begin
            if (StartRTCCmd) begin
                spi_state <= state_LowerCS;
                start_cmd[1] <= start_cmd[0];
            end
        end
        state_LowerCS: begin
            spi_state <= state_Byte0Bit0;
        end
        state_RaiseCS: begin
            spi_state <= state_Idle;
        end
        state_Byte0Bit0+7, state_Byte1Bit0+7, state_Byte2Bit0+7,
                state_Byte3Bit0+7, state_Byte4Bit0+7, state_Byte5Bit0+7,
                state_Byte6Bit0+7, state_Byte7Bit0+7: begin

            if (spi_state != state_Byte0Bit0+7 && ~Ready)
                ready_bit_sclk <= ready_bit_sclk ^ 1;

            if (spi_state == cmd_final_state)
                spi_state <= state_RaiseCS;
            else
                spi_state <= spi_state + 1;
        end
        state_WaitData1, state_WaitData2, state_WaitData3,
                state_WaitData4, state_WaitData5,
                state_WaitData6: begin

            if (~Ready)
                spi_state <= spi_state + 1;
        end
        default: spi_state <= spi_state + 1;
        endcase
    end

    wire[7:0] ShiftRegNext = {shiftreg[6:0], SPIDi};

    reg is_eighth_bit;
    reg wait_for_next_byte;
    always_comb begin
        case (spi_state)
        state_Byte0Bit0+7, state_Byte1Bit0+7, state_Byte2Bit0+7,
                state_Byte3Bit0+7, state_Byte4Bit0+7, state_Byte5Bit0+7,
                state_Byte6Bit0+7, state_Byte7Bit0+7:
            is_eighth_bit = 1;
        default: is_eighth_bit = 0;
        endcase
        case (spi_state)
        state_WaitData1, state_WaitData2, state_WaitData3,
                state_WaitData4, state_WaitData5,
                state_WaitData6:
            wait_for_next_byte = 1;
        default: wait_for_next_byte = 0;
        endcase
    end

    always @(posedge SClk) begin
        if (spi_state == state_LowerCS) begin
            shiftreg <= {4'hF, cmd[3:0]};
        end else if (Shifting) begin
            if (is_eighth_bit || wait_for_next_byte)
                shiftreg <= cmd[0] ? 8'hFF : RTCData;
            else
                shiftreg <= ShiftRegNext;
        end
    end

    always @(posedge SClk) begin
       if (is_eighth_bit && cmd[0]) begin
            data_recv <= ShiftRegNext;
            if (UseWrittenData)
                data_src_RX <= data_src_RX ^ 1;
        end
    end

    /*
        Already let Busy bit go down while the state machine
        isn't in idle state yet so that happens simultaneously
        with the Ready bit to go high for the last byte.
    */
    wire Busy = StartRTCCmd|(spi_state != state_Idle && spi_state != state_RaiseCS);

    assign RTCData = UseWrittenData ? data_written : data_recv;
    assign RTCCtrl = {Ready, 2'b00, Busy, cmd[3:0]};

    reg cs = 1;
    always @(posedge SClk) begin
        if (spi_state == state_LowerCS)
            cs = 0;
        else if (spi_state == state_RaiseCS)
            cs = 1;
    end
    assign nMCUSel = cs;

    assign SPIDo = shiftreg[7];

    assign SPIClkRunning = Shifting;
    assign SPIClkStretch = wait_for_next_byte;

    always @(posedge nWE) begin
        if (SelRTCCtrl
                // only start upon a valid command
                && WriteData[4]
                && WriteData[3:1] != 6
                && WriteData[3:1] != 7) begin
            start_cmd[0] <= start_cmd[0] ^ 1;

            cmd <= WriteData[3:0];
        end
    end

    always @(posedge nWE) begin
        if (SelRTCData) begin
            data_written <= WriteData;
            if (~UseWrittenData)
                data_src_nWE <= data_src_nWE ^ 1;
            
            if (Ready)
                ready_bit_nWE <= ready_bit_nWE ^ 1;
        end
    end

    always @(negedge nOE) begin
        if (SelRTCData) begin
            if (Ready)
                ready_bit_nOE <= ready_bit_nOE ^ 1;
        end
    end
endmodule

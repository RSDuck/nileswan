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
        cmd_RTCResetR,
        cmd_RTCResetW,
        cmd_RTCStatusR,
        cmd_RTCStatusW,
        cmd_RTCData0R,
        cmd_RTCData0W,
        cmd_RTCData4R,
        cmd_RTCData4W,
        cmd_RTCMiscRegR,
        cmd_RTCMiscRegW,
        cmd_RTCNopR,
        cmd_RTCNopW,
        cmd_RTCInvalid6R,
        cmd_RTCInvalid6W,
        cmd_RTCInvalid7R,
        cmd_RTCInvalid7W
    } Command;
    Command cmd;

    reg[1:0] start_cmd = 2'h0;
    reg[1:0] start_rtc_write = 2'h0;
    reg[1:0] start_rtc_read = 2'h0;

    reg[15:0] eeprom_cmd = 16'h0;
    reg[15:0] data_rx = 16'h0;
    reg[15:0] data_tx = 16'h0;

    typedef enum reg[6:0] {
        state_Idle,
        state_LowerCS,
        state_RaiseCS,
        state_Byte0Bit0 = 8,
        state_Byte1Bit0 = 16,
        state_Byte2Bit0 = 24,
        state_Byte3Bit0 = 32,
        state_Byte4Bit0 = 40,
        state_Byte5Bit0 = 48,
        state_Byte6Bit0 = 56,
        state_Byte7Bit0 = 64
    } SPIState;
    SPIState spi_state = state_Idle;
    reg[7:0] shiftreg = 8'h0;

    wire Shifting = spi_state[6:3] != 0;

    wire StartRTCCmd = start_cmd[1] ^ start_cmd[0];
    wire StartRTCWrite = start_rtc_write[1] ^ start_rtc_write[0];
    wire StartRTCRead = start_rtc_read[1] ^ start_rtc_read[0];

    wire Start = StartRTCCmd | StartRTCWrite | StartRTCRead;

    reg[6:0] cmd_final_state;
    always_comb begin
        case (cmd)
        cmd_RTCResetR,
        cmd_RTCResetW: cmd_final_state = state_Byte0Bit0+7;
        cmd_RTCStatusR,
        cmd_RTCStatusW: cmd_final_state = state_Byte1Bit0+7;
        cmd_RTCData0R,
        cmd_RTCData0W: cmd_final_state = state_Byte7Bit0+7;
        cmd_RTCData4R,
        cmd_RTCData4W: cmd_final_state = state_Byte3Bit0+7;
        cmd_RTCMiscRegR,
        cmd_RTCMiscRegW: cmd_final_state = state_Byte2Bit0+7;
        /*cmd_EEPROM_Erase: cmd_final_state = state_Byte1Bit0+7;
        cmd_EEPROM_Write: cmd_final_state = state_Byte3Bit0+7;
        cmd_EEPROM_Read: cmd_final_state = state_Byte3Bit0+7;*/
        default: cmd_final_state = 6'hXX;
        endcase
    end

    reg rtc_data_req = 0;

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

            if (spi_state == cmd_final_state)
                spi_state <= state_RaiseCS;
            // for command reads need to go ahead and read a second byte
            else if (~cmd[0] && spi_state == state_Byte0Bit0+7)
                spi_state <= spi_state + 1;
            else if (StartRTCWrite||StartRTCRead) begin
                spi_state <= spi_state + 1;
                start_rtc_write[1] <= start_rtc_write[0];
                start_rtc_read[1] <= start_rtc_read[0];

                rtc_data_req <= 0;
            end else begin
                rtc_data_req <= 1;
            end
        end
        default: spi_state <= spi_state + 1;
        endcase
    end

    wire[7:0] ShiftRegNext = {shiftreg[6:0], SPIDi};

    always @(posedge SClk) begin
        if (spi_state == state_LowerCS) begin
            shiftreg <= {4'hF, cmd[3:0]};
        end else if (Shifting) begin
            if (spi_state[2:0] == 7)
                shiftreg <= data_tx[7:0];
            else
                shiftreg <= ShiftRegNext;
        end
    end

    always @(posedge SClk) begin
       if (spi_state[2:0] == 7) begin
            data_rx[7:0] <= ShiftRegNext;
        end
    end

    assign RTCData = data_rx[7:0];
    assign RTCCtrl = {rtc_data_req&~(StartRTCRead|StartRTCWrite), 2'b00, Start|(spi_state != state_Idle), cmd[3:0]};

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
    assign SPIClkStretch = rtc_data_req;

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
        if (SelRTCData && RTCCtrl[7]) begin
            data_tx[7:0] <= WriteData;
            start_rtc_write[0] <= start_rtc_write[0] ^ 1;
        end
    end

    always @(negedge nOE) begin
        if (SelRTCData && RTCCtrl[7])
            start_rtc_read[0] <= start_rtc_read[0] ^ 1;
    end
endmodule

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

    input MCUReadyFallingEdge,

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
    reg invalid_cmd = 1'b0;

    reg[1:0] start_cmd = 2'h0;

    reg[7:0] data_written = 8'h0;
    reg[7:0] data_recv = 8'h0;

    reg data_src_nWE = 0;
    reg data_src_RX = 0;
    wire UseWrittenData = data_src_nWE ^ data_src_RX;

    reg[1:0] req_ready_clear_nWE = 2'b0;
    reg[1:0] req_ready_clear_nOE = 2'b0;
    reg[1:0] req_ready_set = 2'b0;

    reg ready = 1'b0;
    reg busy = 1'b0;

    typedef enum reg[4:0] {
        state_Idle,
        state_LowerCS,
        state_RaiseCS,
        state_SendCmd,
        state_WaitCmdAck,
        state_SendData0,
        state_WaitNextData0,
        state_SendData1,
        state_WaitNextData1,
        state_SendData2,
        state_WaitNextData2,
        state_SendData3,
        state_WaitNextData3,
        state_SendData4,
        state_WaitNextData4,
        state_SendData5,
        state_WaitNextData5,
        state_SendData6
    } State;
    State state = state_Idle;

    reg[2:0] spi_bit = 3'h0;
    reg[7:0] shiftreg = 8'h0;

    wire StartRTCCmd = ^start_cmd;

    reg is_final_state;
    always_comb begin
        case (cmd)
        cmd_RTCResetW,
        cmd_RTCResetR: is_final_state = state == state_SendCmd;
        cmd_RTCStatusW,
        cmd_RTCStatusR: is_final_state = state == state_SendData0;
        cmd_RTCData0W,
        cmd_RTCData0R: is_final_state = state == state_SendData6;
        cmd_RTCData4W,
        cmd_RTCData4R: is_final_state = state == state_SendData2;
        cmd_RTCMiscRegW,
        cmd_RTCMiscRegR,
        cmd_RTCNopW,
        cmd_RTCNopR: is_final_state = state == state_SendData1;
        default: is_final_state = 0;
        endcase
    end

    reg falling_edge_registered = 1'b0;
    always @(posedge SClk) begin
        if (state == state_LowerCS || state == state_SendData0) begin
            falling_edge_registered <= 1'b0;
        end
        else if (MCUReadyFallingEdge) begin
            falling_edge_registered <= 1'b1;
        end
    end

    always @(posedge SClk) begin
        if (^req_ready_clear_nWE || ^req_ready_clear_nOE) begin
            ready <= 0;
            req_ready_clear_nWE[1] <= req_ready_clear_nWE[0];
            req_ready_clear_nOE[1] <= req_ready_clear_nOE[0];
        end

        case (state)
        state_Idle: begin
            if (StartRTCCmd) begin
                start_cmd[1] <= start_cmd[0];
                busy <= 1;
                state <= state_LowerCS;
            end
        end
        state_LowerCS: state <= state_SendCmd;
        state_RaiseCS: begin
            if (falling_edge_registered) begin
                state <= state_Idle;
                busy <= 0;
                ready <= 1;
            end
        end
        state_WaitCmdAck: if (falling_edge_registered) state <= state_SendData0;
        state_SendCmd, state_SendData0, state_SendData1,
                state_SendData2, state_SendData3, state_SendData4,
                state_SendData5, state_SendData6:
            if (spi_bit == 3'h7) begin
                if (is_final_state) begin
                    state <= state_RaiseCS;
                end else begin
                    state <= state + 1;
                    if (state != state_SendCmd)
                        ready <= 1;
                end
            end
        state_WaitNextData0, state_WaitNextData1, state_WaitNextData2,
                state_WaitNextData3, state_WaitNextData4,
                state_WaitNextData5:
            if (~ready)
                state <= state + 1;
        default: begin end
        endcase
    end

    wire[7:0] ShiftRegNext = {shiftreg[6:0], SPIDi};

    always @(posedge SClk) begin
        case (state)
        state_LowerCS: shiftreg <= {4'hF, cmd[3:0]};
        state_SendCmd, state_SendData0, state_SendData1,
                state_SendData2, state_SendData3, state_SendData4,
                state_SendData5, state_SendData6: begin
            shiftreg <= ShiftRegNext;

            spi_bit <= spi_bit + 1;
        end
        state_WaitCmdAck, state_WaitNextData0, state_WaitNextData1,
                state_WaitNextData2, state_WaitNextData3, state_WaitNextData4,
                state_WaitNextData5:
            shiftreg <= cmd[0] ? 8'hFF : RTCData;
        default: begin end
        endcase
    end

    always @(posedge SClk) begin
        case (state)
        state_SendCmd, state_SendData0, state_SendData1,
                state_SendData2, state_SendData3, state_SendData4,
                state_SendData5, state_SendData6:
            if (spi_bit == 3'h7 && cmd[0]) begin
                data_recv <= ShiftRegNext;
                if (UseWrittenData)
                    data_src_RX <= data_src_RX ^ 1;
            end
        default: begin end
        endcase
    end

    wire Busy = StartRTCCmd|busy;
    wire Ready = ready & ~(^req_ready_clear_nOE | ^req_ready_clear_nWE);

    assign RTCData = UseWrittenData ? data_written : data_recv;
    assign RTCCtrl = {Ready, 2'b00, Busy|invalid_cmd, cmd[3:0]};

    reg cs = 1;
    always @(posedge SClk) begin
        if (state == state_LowerCS)
            cs = 0;
        else if (state == state_RaiseCS)
            cs = 1;
    end
    assign nMCUSel = cs;

    assign SPIDo = shiftreg[7];

    assign SPIClkRunning = state != state_Idle && state != state_LowerCS && state != state_RaiseCS;
    reg clock_stretch;
    assign SPIClkStretch = clock_stretch;
    always_comb begin
        case (state)
        state_WaitCmdAck, state_WaitNextData0, state_WaitNextData1,
                state_WaitNextData2, state_WaitNextData3,
                state_WaitNextData4, state_WaitNextData5:
            clock_stretch = 1;
        default: clock_stretch = 0;
        endcase
    end

    always @(posedge nWE) begin
        if (SelRTCCtrl && ~Busy) begin
            cmd <= WriteData[3:0];
            invalid_cmd <= 0;
            if (WriteData[4]) begin
                // only start upon a valid command
                if (WriteData[3:1] != 6 && WriteData[3:1] != 7) begin
                    start_cmd[0] <= start_cmd[0] ^ 1;
                    if (~^req_ready_clear_nWE)
                    req_ready_clear_nWE[0] <= req_ready_clear_nWE[0] ^ 1;
                end else begin
                    invalid_cmd <= 1;
                end
            end
        end

        if (SelRTCData) begin
            data_written <= WriteData;
            if (~UseWrittenData)
                data_src_nWE <= data_src_nWE ^ 1;

            if ((~cmd[0] | ~Busy) && ~^req_ready_clear_nWE)
                req_ready_clear_nWE[0] <= req_ready_clear_nWE[0] ^ 1;
        end
    end

    always @(posedge nOE) begin
        if (SelRTCData && (cmd[0] | ~Busy)) begin
            req_ready_clear_nOE[0] <= req_ready_clear_nOE[0] ^ 1;
        end
    end
endmodule

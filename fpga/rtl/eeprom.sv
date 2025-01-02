module EEPROM (
    input SClk,
    input nWE,
    input nOE,

    input[7:0] WriteData,

    input[1:0] EEPROMSize,

    input SelSerialCtrl,
    input SelSerialComLo,
    input SelSerialComHi,
    input SelSerialDataLo,
    input SelSerialDataHi,

    output[7:0] SerialCtrl,
    output[15:0] SerialCom,
    output[15:0] SerialData,

    output SPIDo,
    output SPISel,
    output SPIClkRunning);

    (* blockram *)
    reg[15:0] memory[1024];

    typedef enum reg[1:0] {
        eepromSize_128B,
        eepromSize_1KB,
        eepromSize_2KB,
        eepromSize_NoEEPROM
    } EEPROMSizeTypes;

    typedef enum reg[1:0] {
        cmd_Write,
        cmd_Erase,
        cmd_EraseAll,
        cmd_WriteAll
    } Cmd;

    reg[15:0] command = 16'h0;
    reg[15:0] serial_data_out = 16'h0;

    reg[4:0] op;
    reg[9:0] addr_mask;
    always_comb begin
        case (EEPROMSize)
        eepromSize_128B: op = command[8:4];
        default: op = command[12:8];
        endcase

        case (EEPROMSize)
        eepromSize_128B: addr_mask = 10'h3F;
        eepromSize_1KB: addr_mask = 10'h1FF;
        default: addr_mask = 10'h3FF;
        endcase
    end

    reg internal_write_done = 0, internal_write_start = 0;
    wire InternalWriteBusy = internal_write_done ^ internal_write_start;

    // command abort support is currently commented out
    // due to it being very obscure and untested

    /*// in case an abort and a new command is issued between two serial
    // cycles to reset the write counter
    reg interal_write_restart_done = 0, internal_write_restart = 0;
    wire NeedRestart = interal_write_restart_done ^ internal_write_restart;*/

    Cmd cmd = cmd_Write;

    reg[9:0] write_counter = 0;

    reg WriteCounterLastValue;
    always_comb begin
        if (cmd == cmd_Write)
            WriteCounterLastValue = write_counter == 10'h21;
        else if (cmd == cmd_EraseAll)
            WriteCounterLastValue = write_counter == 10'h11;
        else
            WriteCounterLastValue = (write_counter & addr_mask) == addr_mask;
    end

    always @(posedge SClk) begin
        if (InternalWriteBusy)
            write_counter <= write_counter + 1;
        else
            write_counter <= 0;
    end

    always @(posedge SClk) begin
        if (InternalWriteBusy && WriteCounterLastValue) begin
            internal_write_done <= internal_write_start;
        end
    end

    reg[9:0] write_addr;
    reg[15:0] write_data;
    always_comb begin
        if (cmd == cmd_EraseAll || cmd == cmd_WriteAll) begin
            write_addr = write_counter & addr_mask;
        end else begin
            write_addr = command[9:0] & addr_mask;
        end

        if (cmd == cmd_Erase || cmd == cmd_EraseAll) begin
            write_data = 16'hFFFF;
        end else begin
            write_data = serial_data_out;
        end
    end

    always @(posedge SClk) begin
        if (InternalWriteBusy && EEPROMSize != eepromSize_NoEEPROM)
            memory[write_addr] <= write_data;
    end

    reg spi_clk_enable = 0;
    reg spi_sel = 1;
    assign SPIClkRunning = spi_clk_enable;
    assign SPISel = spi_sel;

    always @(posedge SClk) begin
        case (write_counter)
        10'h1: begin
            spi_clk_enable <= 1;
            spi_sel <= 0;
        end
        10'h11: if (cmd == cmd_Erase || cmd == cmd_EraseAll) begin
            spi_clk_enable <= 0;
        end
        10'h12: if (cmd == cmd_Erase || cmd == cmd_EraseAll) begin
            spi_sel <= 1;
        end
        10'h21: spi_clk_enable <= 0;
        10'h22: spi_sel <= 1;
        default: begin end
        endcase
    end

    reg spi_out;
    assign SPIDo = spi_out;
    always_comb begin
        case (write_counter)
        10'h2: spi_out = command[15];
        10'h3: spi_out = command[14];
        10'h4: spi_out = command[13];
        10'h5: spi_out = command[12];
        10'h6: spi_out = command[11];
        10'h7: spi_out = command[10];
        10'h8: spi_out = command[9];
        10'h9: spi_out = command[8];
        10'hA: spi_out = command[7];
        10'hB: spi_out = command[6];
        10'hC: spi_out = command[5];
        10'hD: spi_out = command[4];
        10'hE: spi_out = command[3];
        10'hF: spi_out = command[2];
        10'h10: spi_out = command[1];
        10'h11: spi_out = command[0];
        10'h12: spi_out = serial_data_out[15];
        10'h13: spi_out = serial_data_out[14];
        10'h14: spi_out = serial_data_out[13];
        10'h15: spi_out = serial_data_out[12];
        10'h16: spi_out = serial_data_out[11];
        10'h17: spi_out = serial_data_out[10];
        10'h18: spi_out = serial_data_out[9];
        10'h19: spi_out = serial_data_out[8];
        10'h1A: spi_out = serial_data_out[7];
        10'h1B: spi_out = serial_data_out[6];
        10'h1C: spi_out = serial_data_out[5];
        10'h1D: spi_out = serial_data_out[4];
        10'h1E: spi_out = serial_data_out[3];
        10'h1F: spi_out = serial_data_out[2];
        10'h20: spi_out = serial_data_out[1];
        10'h21: spi_out = serial_data_out[0];
        default: spi_out = 1'bX;
        endcase
    end

    // handle it separately so that we can do the block ram read
    // directly on /WE
    wire DoRead = SelSerialCtrl && ~InternalWriteBusy && WriteData[7:4] == 4'b0001 && op[3:2] == 2'b10;

    reg invalid_serial_read = 0;
    wire[9:0] read_addr = command[9:0] & addr_mask;
    reg[15:0] serial_data_in;
    always @(posedge nWE) begin
        if (DoRead)
            serial_data_in <= memory[read_addr];
    end

    always @(posedge nWE) begin
        if (DoRead)
            invalid_serial_read <= ~op[4] || EEPROMSize == eepromSize_NoEEPROM;
    end

    reg bugged_done_bit = 0;
    reg write_protect = 1;

    assign SerialData = invalid_serial_read ? 16'hFFFF : serial_data_in;
    assign SerialCom = command;
    assign SerialCtrl = {6'h0, ~InternalWriteBusy, bugged_done_bit};

    always @(posedge nWE) begin
        /*if (SelSerialCtrl && InternalWriteBusy) begin
            case (WriteData[7:4])
            4'b1000: begin
                bugged_done_bit <= 1;
                internal_write_start <= internal_write_done;
                internal_write_restart <= interal_write_restart_done ^ 1;
            end
            endcase
        end else */if (SelSerialCtrl && ~InternalWriteBusy) begin
            if (DoRead)
                bugged_done_bit <= 1;

            case (WriteData[7:4])
            4'b0010: begin
                casez (op)
                5'b101??: begin
                    bugged_done_bit <= 0;
                    cmd <= cmd_Write;
                    internal_write_start <= internal_write_start ^ ~write_protect;
                end
                5'b10001: begin
                    bugged_done_bit <= 0;
                    cmd <= cmd_WriteAll;
                    internal_write_start <= internal_write_start ^ ~write_protect;
                end
                default: begin end
                endcase
            end
            4'b0100: begin
                casez (op)
                5'b111??: begin
                    bugged_done_bit <= 0;
                    cmd <= cmd_Erase;
                    internal_write_start <= internal_write_start ^ ~write_protect;
                end
                5'b10010: begin
                    bugged_done_bit <= 0;
                    cmd <= cmd_EraseAll;
                    internal_write_start <= internal_write_start ^ ~write_protect;
                end
                5'b10000: write_protect <= 1; // write/erase disable
                5'b10011: write_protect <= 0; // write/erase enable
                default: begin end
                endcase
            end
            default: begin end // none or more than two bits are invalid
            endcase
        end
    end

    always @(posedge nWE) begin
        if (SelSerialComLo)
            command[7:0] <= WriteData;
        if (SelSerialComHi)
            command[15:8] <= WriteData;
        if (SelSerialDataLo)
            serial_data_out[7:0] <= WriteData;
        if (SelSerialDataHi)
            serial_data_out[15:8] <= WriteData;
    end

endmodule
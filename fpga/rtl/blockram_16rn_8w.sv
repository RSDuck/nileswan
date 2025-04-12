module BlockRAM16RN_8W (
    input ReadClk,
    input ReadEnable,
    input[9:0] ReadAddr,
    output[15:0] ReadData,

    input WriteClk,
    input WriteEnable,
    input[10:0] WriteAddr,
    input[7:0] WriteData);

    (* ram_style = "block" *)
    reg[7:0] memory[0:2047];

    integer i;
    initial begin
        for (i = 0; i < 2048; i++) begin
            memory[i] <= 8'h0;
        end
    end

    reg[15:0] read_data;
    assign ReadData = read_data;

    always @(negedge ReadClk) begin
        if (ReadEnable) begin
            read_data[7:0] <= memory[{ReadAddr,1'b0}];
            read_data[15:8] <= memory[{ReadAddr,1'b1}];
        end
    end

    always @(posedge WriteClk) begin
        if (WriteEnable)
            memory[WriteAddr] <= WriteData;
    end
endmodule

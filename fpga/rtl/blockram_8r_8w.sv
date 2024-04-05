module BlockRAM8R_8W (
    input ReadClk,
    input ReadEnable,
    input[9:0] ReadAddr,
    output[7:0] ReadData,

    input WriteClk,
    input WriteEnable,
    input[9:0] WriteAddr,
    input[7:0] WriteData);

    (* ram_style = "block" *)
    reg[7:0] memory[0:1023];

    integer i;
    initial begin
        for (i = 0; i < 1024; i++) begin
            memory[i] <= 8'h0;
        end
    end

    reg[7:0] read_data;
    assign ReadData = read_data;

    always @(posedge ReadClk) begin
        if (ReadEnable)
            read_data <= memory[ReadAddr];
    end

    always @(posedge WriteClk) begin
        if (WriteEnable)
            memory[WriteAddr] <= WriteData;
    end
endmodule

module IPCRAM (
    input nOE,
    input nWE,
    input Sel,
    input[8:0] AddrLo,

    output[7:0] ReadData,
    input[7:0] WriteData
);
    (* ram_style = "block" *)
    reg[7:0] memory[0:511];

    reg[7:0] read_data;
    assign ReadData = read_data;

    always @(posedge nWE) begin
        if (Sel)
            memory[AddrLo] <= WriteData;
    end

    always @(negedge nOE) begin
        if (Sel)
            read_data <= memory[AddrLo];
    end
endmodule
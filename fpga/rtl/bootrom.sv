
module BootROM (
    input Sel,
    input nOE,
    input[8:0] AddrLo,

    input BypassSplash,

    output[15:0] ReadData
);

    (* ram_style = "block" *)
    reg[15:0] memory[0:255];

    initial begin
`ifdef YOSYS
        $readmemh("build/bram_init.asc", memory);
`else
        $readmemh("../software/ipl0/ipl0.asc", memory);
`endif
    end

    reg[15:0] read_data;

    assign ReadData[14:0] = read_data[14:0];
    assign ReadData[15] = BypassSplash && AddrLo[8:1] == 8'hFA ? 1'b1 : read_data[15];

    always @(negedge nOE) begin
        if (Sel)
            read_data <= memory[AddrLo[8:1]];
    end
endmodule

module BootROM (
    input nOE,
    input[8:0] AddrLo,

    output[15:0] ReadData
);

    (* ram_style = "block" *)
    reg[15:0] memory[0:255];

    initial begin
`ifdef YOSYS
        $readmemh("build/bram_init.asc", memory);
`else
        $readmemh("build/ipl0.asc", memory);
`endif
    end

    reg[15:0] read_data;
    assign ReadData = read_data;

    always @(negedge nOE)
        read_data <= memory[AddrLo[8:1]];

endmodule

module BlockRam (
    input nOE,
    input[7:0] AddrLo,

    output[15:0] ReadData
);

    SB_RAM40_4KNRNW #(
        .READ_MODE(0),
`ifdef YOSYS
        .INIT_FILE("build/bram_init.asc")
`else
        .INIT_FILE("build/ipl0.asc")
`endif
    ) memory (
        .RCLKN(nOE),
        .RDATA(ReadData),
        .RADDR({1'b0, AddrLo[7:1]}),
        .WADDR({1'b0, AddrLo[7:1]}),
        .RE(1'b1)
    );

endmodule
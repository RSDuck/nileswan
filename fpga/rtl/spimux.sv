module SPIMux #(
    parameter SIZE) (
    input Clk,

    input[SIZE-1:0] ClockRunning,
    input[SIZE-1:0] ClockStretch,
    input[SIZE-1:0] InSPIDo,
    input[SIZE-1:0] InSPISel,

    output OutSPIDo,
    output OutSPIClk);

    wire Outbit = |(InSPIDo&~InSPISel);

    SB_IO #(
        .PIN_TYPE(6'b010001), // PIN_OUTPUT_DDR
        .IO_STANDARD("SB_LVCMOS")
    ) spi_clk_iob (
        .PACKAGE_PIN(OutSPIClk),
        .D_OUT_0(|ClockRunning),
        .D_OUT_1(|ClockStretch),
        .OUTPUT_CLK(Clk),
        .OUTPUT_ENABLE(1'b1),
        .CLOCK_ENABLE(1'b1)
    );
    SB_IO #(
        .PIN_TYPE(6'b010001), // PIN_OUTPUT_DDR
        .IO_STANDARD("SB_LVCMOS")
    ) spi_do_iob (
        .PACKAGE_PIN(OutSPIDo),
        .D_OUT_0(Outbit),
        .D_OUT_1(Outbit),
        .OUTPUT_CLK(Clk),
        .OUTPUT_ENABLE(1'b1),
        .CLOCK_ENABLE(1'b1)
    );

endmodule
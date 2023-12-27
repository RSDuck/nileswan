module SPI (
    input Clk,
    input[7:0] DataOut,
    output[7:0] DataIn,
    output Busy,
    input Start,
    input KeepCS,

    output SPI_Do,
    input SPI_Di,
    output SPI_Clk,
    output SPI_Cs
);

    // State machine
    typedef enum reg[3:0] {
        state_Idle = 4'h1,
        state_Shift0 = 4'h8,
        state_Shift1 = 4'h9,
        state_Shift2 = 4'hA,
        state_Shift3 = 4'hB,
        state_Shift4 = 4'hC,
        state_Shift5 = 4'hD,
        state_Shift6 = 4'hE,
        state_Shift7 = 4'hF,
        // since CS is phase shifted by
        // -180 degrees (see below), directly lifting CS
        // on the positive edge would mean that
        // would result in CS being high already
        // during the second half of the last cycle
        state_Coda = 4'h0
    } State;

    State state = state_Idle;
    wire StateIdle = ~state[3] & state[0];
    wire StateCoda = ~state[3] & ~state[0];
    wire StateShiftOut = state[3];

    reg[7:0] shiftreg = 0;

    reg cs_keep = 0;
    wire cs = ~(~StateIdle | cs_keep);

    assign DataIn = shiftreg;
    assign Busy = ~StateIdle;

    always @(posedge Clk) begin
        if (StateIdle && Start)
            state <= state_Shift0;

        if (~StateIdle)
            state <= state + 1;
    end

    always @(posedge Clk)
        if (StateIdle && Start)
            cs_keep <= KeepCS;

    always @(posedge Clk) begin
        if (StateIdle && Start)
            shiftreg <= DataOut;
        if (StateShiftOut)
            shiftreg <= {shiftreg[6:0], SPI_Di};
    end

    SB_IO #(
        .PIN_TYPE(6'b010001), // PIN_OUTPUT_DDR
        .IO_STANDARD("SB_LVCMOS")
    ) spi_clk_iob (
        .PACKAGE_PIN(SPI_Clk),
        .D_OUT_0(StateShiftOut),
        .D_OUT_1(1'b0),
        .OUTPUT_CLK(Clk),
        .OUTPUT_ENABLE(1'b1),
        .CLOCK_ENABLE(1'b1)
    );

    /*
        Note that DDR buffers don't latch both output values
        on the positive edge. Instead they latch each value
        at the edge it should be output.

        This means a flip flop set on the positive edge
        (like our shift register here) will already be output
        on the next negative edge (so within the same period!).

        On the next positive edge the same value will be output again.
        (because the new value is just being latched into shiftreg,
            not into the output buffer yet)

        This whole phenomenom makes the DDR output buffers
        slightly counter intuitive, because D_OUT_1 will be output
        before D_OUT_0 assuming the values going to both flip flops
        are both latched on the rising edge.
    */
    SB_IO #(
        .PIN_TYPE(6'b010001), // PIN_OUTPUT_DDR
        .IO_STANDARD("SB_LVCMOS")
    ) spi_do_iob (
        .PACKAGE_PIN(SPI_Do),
        .D_OUT_0(shiftreg[7]),
        .D_OUT_1(shiftreg[7]),
        .OUTPUT_CLK(Clk),
        .OUTPUT_ENABLE(1'b1),
        .CLOCK_ENABLE(1'b1)
    );

    SB_IO #(
        .PIN_TYPE(6'b010001), // PIN_OUTPUT_DDR
        .IO_STANDARD("SB_LVCMOS")
    ) spi_cs_iob (
        .PACKAGE_PIN(SPI_Cs),
        .D_OUT_0(cs),
        .D_OUT_1(cs),
        .OUTPUT_CLK(Clk),
        .OUTPUT_ENABLE(1'b1),
        .CLOCK_ENABLE(1'b1)
    );
endmodule
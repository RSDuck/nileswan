module ClockMux (
    input ClkA,
    input ClkB,

    input ClkSel,

    output OutClk
);
    /*
        See:
        https://vlsitutorials.com/glitch-free-clock-mux/
        and https://www.intel.com/content/www/us/en/docs/programmable/683082/21-3/clock-multiplexing.html

        The way this works is a bit hard to understand, because of it's circular nature.
    */

    reg[2:0] ff_sel_a = 3'h0;
    reg[2:0] ff_sel_b = 3'h0;

    always @(posedge ClkA)
        ff_sel_a[1:0] <= {ff_sel_a[0], ~ClkSel & ~ff_sel_b[2]};

    always @(negedge ClkA)
        ff_sel_a[2] <= ff_sel_a[1];

    always @(posedge ClkB)
        ff_sel_b[1:0] <= {ff_sel_b[0], ClkSel & ~ff_sel_a[2]};

    always @(negedge ClkB)
        ff_sel_b[2] <= ff_sel_b[1];

    // prevent glitches by gating each clock with its own logic cell
    // this way only one of the inputs to OR gate changes at a time
    wire gated_a, gated_b;
    SB_LUT4 #(.LUT_INIT(16'b1000100010001000)) a_gate (
        .O(gated_a),
        .I0(ff_sel_a[2]),
        .I1(ClkA));
    SB_LUT4 #(.LUT_INIT(16'b1000100010001000)) b_gate (
        .O(gated_b),
        .I0(ff_sel_b[2]),
        .I1(ClkB));

    assign OutClk = gated_a | gated_b;
endmodule

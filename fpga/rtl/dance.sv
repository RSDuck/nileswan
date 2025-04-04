module Dance (
    input[7:0] AddrLo,
    input[3:0] AddrHi,
    input SClk,
    output MBC,
    output MBCSeqStart);

    wire address_change = AddrLo[3:0] == 4'h5 && AddrHi == 4'hA;

    reg read_A5 = 0;
    reg[4:0] state = 5'h0;

    assign MBCSeqStart = state == 5'h1;

    always @(posedge SClk) begin
        if (address_change)
            read_A5 <= 1;
    end

    always @(posedge SClk) begin
        if (state != 5'h1F && read_A5)
            state <= state + 1;
    end

    reg mbc;
    assign MBC = mbc;
    always_comb begin
        case (state)
        1: mbc = 1;
        2: mbc = 1;
        3: mbc = 1;
        4: mbc = 0;
        5: mbc = 0;
        6: mbc = 0;
        7: mbc = 0;
        8: mbc = 0;
        9: mbc = 0;
        10: mbc = 1;
        11: mbc = 0;
        12: mbc = 1;
        13: mbc = 0;
        14: mbc = 0;
        15: mbc = 0;
        16: mbc = 1;
        17: mbc = 0;
        18: mbc = 1;
        19: mbc = 0;
        20: mbc = 0;
        21: mbc = 0;
        default: mbc = 1;
        endcase
    end
endmodule
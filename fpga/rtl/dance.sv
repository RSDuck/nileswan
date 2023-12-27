module Dance (
    input[7:0] AddrLo,
    input[3:0] AddrHi,
    input SClk,
    output MBC);

    reg read_A5 = 1'b0;
    reg[21:0] unlock_cur_state = UnlockBits;

    localparam[22:0] UnlockBits = 23'b000101000101000000111;

    assign MBC = unlock_cur_state[0];

    always @(posedge SClk) begin
        if (~read_A5 && AddrLo[3:0] == 4'h5 && AddrHi == 4'hA) begin
            read_A5 <= 1'h1;
        end 
    end

    always @(posedge SClk) begin
        if (read_A5) begin
            unlock_cur_state <= {1'b1, unlock_cur_state[21:1]};
        end
    end
endmodule
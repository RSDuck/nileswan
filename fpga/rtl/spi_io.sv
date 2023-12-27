`include "spi.sv"

module SPI_IO (
    input FastClk,
    input nWE,

    input OE_WE_Rising_FastClk,

    output[7:0] ReadSPIData,
    output[7:0] ReadSPICnt,

    input[7:0] WriteData,
    input WriteSPIData,
    input WriteSPICnt,

    output SPI_Cs,
    output SPI_Clk,
    output SPI_Do,
    input SPI_Di
);
    /*
        Besides providing the register interface for SPI
        this module also handles the synchronisation
        from the asynchronous clock domain (Wonderswan I/O access)
        to FastCLK.
    */

    wire[7:0] data_in_fastclk;

    reg[7:0] data_out_async;
    reg keep_cs_async = 0;

    wire busy_fastclk;

    reg[2:0] start_toggle_fastclk = 0;
    reg start_toggle_async = 0;

    always @(posedge FastClk) begin
        start_toggle_fastclk <= {start_toggle_fastclk[1:0], start_toggle_async};
    end

    /*
        SPI latches DataOut and KeepCS still during the write cycle
        (with start_toggle_fastclk as a guard)
        so them changing later shouldn't be an issue for the SPI operation.
    */
    SPI spi (
        .Clk(FastClk),
        .DataOut(data_out_async),
        .DataIn(data_in_fastclk),
        .Busy(busy_fastclk),
        .Start(start_toggle_fastclk[2] ^ start_toggle_fastclk[1]),

        .SPI_Cs(SPI_Cs),
        .SPI_Clk(SPI_Clk),
        .SPI_Do(SPI_Do),
        .SPI_Di(SPI_Di),

        .KeepCS(keep_cs_async)
    );

    /*
        This is a bit weird:

        We sync these variables back into
        the asynchronous context from the clock domain
        of the fast clock.

        Since there is no periodic clock in this domain
        the usual sychronisation methods don't work.

        What we do instead is sample for rising edge
        of /OE or /WE in the fast clock domain and
        then update the flip flops used by the asynchronous
        domain.

        Neither the Wonderswan nor the asynchronous I/O writes
        (which are derived from the Wonderswan's clock) should
        latch at this point and there should be enough time
        for all signals to propagate.

        Just need to make sure to keep a watch on...
        FastClk$SB_IO_IN_$glb_clk -> posedge IOWrite_$glb_clk
    */
    reg[7:0] data_in_async = 0;
    reg busy_toggle_async = 0;

    always @(posedge FastClk) begin
        if (OE_WE_Rising_FastClk & ~busy_fastclk) begin
            data_in_async <= data_in_fastclk;
            busy_toggle_async <= start_toggle_fastclk[2];
        end
    end

    wire busy_async = start_toggle_async ^ busy_toggle_async;

    assign ReadSPIData = data_in_async;
    assign ReadSPICnt = {6'h0, keep_cs_async, busy_async};

    always @(posedge nWE) begin
        if (WriteSPICnt) begin
            keep_cs_async <= WriteData[1];
        end

        if (WriteSPIData && !busy_async) begin
            data_out_async <= WriteData;
            start_toggle_async <= start_toggle_async ^ 1;
        end
    end

endmodule
`define make_spi_dev(name, camel_name, cs, clk, poci, pico) \
    bit ``name``_rx_queue[$]; \
    bit ``name``_tx_queue[$]; \
    \
    always @(posedge ``clk``) begin \
        if (~``cs``) \
            ``name``_rx_queue.push_back(``pico``); \
    end \
    always @(negedge ``clk`` or negedge ``cs``) begin \
        if (~``cs``) begin \
            ``poci`` = ``name``_tx_queue.pop_front(); \
        end \
    end \
    \
    task push``camel_name``Tx(input[7:0] data); \
        for (i = 7; i >= 0; i--) begin \
            ``name``_tx_queue.push_back(data[i]); \
        end \
    endtask \
    \
    task compare``camel_name``RX(input[7:0] should); \
        reg[7:0] got; \
        for (i = 7; i >= 0; i--) begin \
            got[i] = ``name``_rx_queue.pop_front(); \
        end \
        assert (got == should) \ 
        else   $error("SPI %s data mismatch (expected %x got %x)", "`camel_name`", should, got); \
    endtask

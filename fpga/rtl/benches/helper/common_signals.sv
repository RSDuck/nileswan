
localparam fastclk_half_period = 1000 / 25 / 2;
reg clk = 0;

// 25 MHz clock
always #(fastclk_half_period) clk = ~clk;

// serial clock
localparam sclk_half_period = 1000000 / 384 / 2;
reg sclk = 0;
always #(sclk_half_period) sclk = ~sclk;

localparam swan_clock_period = 1000 / 6;

reg nOE = 1, nWE = 1, nIO = 1;

reg[7:0] write_data;

task writeReg(input[7:0] data);
    write_data = data;
    #(swan_clock_period/2);
    nIO = 0;
    nWE = 0;
    #(swan_clock_period);
    nWE = 1;
    #(swan_clock_period/2);
    nIO = 1;
endtask

task readReg();
    #(swan_clock_period/2);
    nIO = 0;
    nOE = 0;
    #(swan_clock_period);
    nOE = 1;
    #(swan_clock_period/2);
    nIO = 1;
endtask
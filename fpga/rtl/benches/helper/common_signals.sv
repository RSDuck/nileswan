
localparam fastclk_half_period = 1000 / 25 / 2;
reg clk = 0;

// 25 MHz clock
always #(fastclk_half_period) clk = ~clk;

// serial clock
localparam sclk_half_period = 1000000 / 384 / 2;
reg sclk = 0;
always #(sclk_half_period) sclk = ~sclk;

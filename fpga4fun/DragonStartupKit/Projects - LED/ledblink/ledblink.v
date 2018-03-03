// (c) fpga4fun.com KNJN LLC - 2004, 2005

module ledblink(clk, LED, LED2);
input clk;
output LED, LED2;

reg [23:0] cnt;
always @(posedge clk) cnt<=cnt+1;

assign LED = cnt[20] & ~cnt[22];
assign LED2 = cnt[21] & ~cnt[23];

endmodule

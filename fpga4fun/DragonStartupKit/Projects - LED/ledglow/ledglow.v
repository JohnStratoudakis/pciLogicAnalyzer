// (c) fpga4fun.com KNJN LLC - 2004, 2005

module ledglow(clk, LED);
input clk;
output [2:0] LED;

reg [23:0] cnt;
always @(posedge clk) cnt<=cnt+1;

wire [3:0] PWM_input = cnt[23] ? cnt[22:19] : ~cnt[22:19];
reg [4:0] PWM;
always @(posedge clk) PWM <= PWM[3:0]+PWM_input;

assign LED[0] =  PWM[4];
assign LED[1] = ~PWM[4];
assign LED[2] = cnt[23];

endmodule

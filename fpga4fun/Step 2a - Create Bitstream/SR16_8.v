module SR16_8(d, q, clk);
input clk;
input [7:0] d;
output [7:0] q;

SR16 SR_0(.d(d[0]), .q(q[0]), .clk(clk));
SR16 SR_1(.d(d[1]), .q(q[1]), .clk(clk));
SR16 SR_2(.d(d[2]), .q(q[2]), .clk(clk));
SR16 SR_3(.d(d[3]), .q(q[3]), .clk(clk));
SR16 SR_4(.d(d[4]), .q(q[4]), .clk(clk));
SR16 SR_5(.d(d[5]), .q(q[5]), .clk(clk));
SR16 SR_6(.d(d[6]), .q(q[6]), .clk(clk));
SR16 SR_7(.d(d[7]), .q(q[7]), .clk(clk));

endmodule



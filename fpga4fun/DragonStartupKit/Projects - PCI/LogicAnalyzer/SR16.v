module SR16(d, q, clk);
input d, clk;
output q;

reg [15:0] data;
//assign q = data[15];
assign q = data[3];
always @(posedge clk) data[15:0] <= {data[14:0], d};

endmodule
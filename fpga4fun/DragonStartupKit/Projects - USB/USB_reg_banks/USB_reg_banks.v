// Example on how to implement "USB registers" in the Dragon board
// (c) fpga4fun.com KNJN LLC - 2004, 2005
//
// This code creates a few 16 bits registers in the FPGA, writable from USB
// Each register has an address and is individually addressable
// For example, here:
//  block1_regA is located at address 0x0000
//  block1_regB is located at address 0x0002
//  block2_regA is located at address 0x1000
//
// To write these registers, send a 4 bytes USB packets to Dragon
//  USB Packet = 2 bytes of address plus 2 bytes of data
// For example, to write 0x1234 to block2_regA, send "00 10 34 12"
// We can send packets up to 64 bytes (2 bytes address, up to 62 bytes data)
// We could also extend the idea to write to blockram instead of registers

// In C for example, that gives us:
//
// void USB_WriteDWord(DWORD dw)
// {
//   USB_Write(&dw, 4);
// }
//
// USB_WriteDWord(0x80341000);	// sending "00 10 34 80" (little Endian) to write 0x8034 in block2_regA


module USB_reg_banks(clk, USB_FWRn, USB_D, LED);
input clk, USB_FWRn;
input [7:0] USB_D;
output [2:0] LED;

// We use the first 2 bytes of a block as an address, the next (up to 62 bytes) as data
// First we detect the end of USB blocks by using a timeout
wire [7:0] RxD_data = USB_D;
wire RxD_data_ready = ~USB_FWRn;
reg [3:0] USB_blockcnt;
wire RxD_idle = USB_blockcnt[3];  // end of block is when no data received for 8 clocks
always @(posedge clk) if(RxD_data_ready) USB_blockcnt<=0; else if(~RxD_idle) USB_blockcnt<=USB_blockcnt[2:0]+1;

// Now count the first block bytes
reg [1:0] RxD_blockstart;  // count 0 for the first byte, 1 for the second byte, and 2 for the remaining bytes
always @(posedge clk) if(RxD_data_ready & ~RxD_blockstart[1]) RxD_blockstart<=RxD_blockstart+1; else if(RxD_idle) RxD_blockstart<=0;

// Catch the 2 address bytes
reg [15:0] ramwr_adr;
always @(posedge clk)
if(RxD_data_ready)
case(RxD_blockstart[1])
	1'b0: if(RxD_blockstart[0]) ramwr_adr[15:8]<=RxD_data; else ramwr_adr[7:0]<=RxD_data;
	1'b1: ramwr_adr <= ramwr_adr+1;
endcase

// Define registers blocks 1
wire block1_wr = RxD_data_ready & RxD_blockstart[1] & (ramwr_adr[15:12]==0);
reg [15:0] block1_regA;
always @(posedge clk) if(block1_wr & ramwr_adr[11:0]==12'h000) block1_regA[ 7:0] <= RxD_data;
always @(posedge clk) if(block1_wr & ramwr_adr[11:0]==12'h001) block1_regA[15:8] <= RxD_data;
reg [15:0] block1_regB;
always @(posedge clk) if(block1_wr & ramwr_adr[11:0]==12'h002) block1_regB[ 7:0] <= RxD_data;
always @(posedge clk) if(block1_wr & ramwr_adr[11:0]==12'h003) block1_regB[15:8] <= RxD_data;

// Define registers blocks 2
wire block2_wr = RxD_data_ready & RxD_blockstart[1] & (ramwr_adr[15:12]==1);
reg [15:0] block2_regA;
always @(posedge clk) if(block2_wr & ramwr_adr[11:0]==12'h000) block2_regA[ 7:0] <= RxD_data;
always @(posedge clk) if(block2_wr & ramwr_adr[11:0]==12'h001) block2_regA[15:8] <= RxD_data;

// Let's use a few register bits to control the LEDs...
assign LED[0] = block1_regA[0];
assign LED[1] = block1_regB[8];
assign LED[2] = block2_regA[15];

endmodule
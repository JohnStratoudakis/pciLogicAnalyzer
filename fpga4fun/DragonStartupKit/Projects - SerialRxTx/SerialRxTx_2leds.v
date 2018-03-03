// This example shows how to receive and send on a serial port
// For Xylo/Dragon, we use the secondary connector with a MAX232 adaptor board

// For any character "X" received from the PC, the character "X+1" is sent back
// So for example, if the PC sends "A", the character "B" is sent back to the PC
// Any character received also controls the LEDs

// Note: After configuring the FPGA, you can use FPGAconf's integrated terminal (CTRL-T) 
// to send and received data on the PC's serial port

module SerialRxTx_2leds(clk, RxD, TxD, LED);
input clk;
input RxD;
output TxD;

parameter NumberOfLEDs = 2;
output [NumberOfLEDs-1:0] LED;

parameter ClkFrequency = 24000000;	// make sure this matches the clock frequency on your board

wire RxD_data_ready;
wire [7:0] RxD_data;
async_receiver RX(.clk(clk), .RxD(RxD), .RxD_data_ready(RxD_data_ready), .RxD_data(RxD_data));
defparam RX.ClkFrequency=ClkFrequency;

reg [NumberOfLEDs-1:0] LED;
always @(posedge clk) if(RxD_data_ready) LED <= RxD_data[NumberOfLEDs-1:0];

wire [7:0] outputdata = RxD_data+1;
async_transmitter TX(.clk(clk), .TxD(TxD), .TxD_start(RxD_data_ready), .TxD_data(outputdata));
defparam TX.ClkFrequency=ClkFrequency;

endmodule
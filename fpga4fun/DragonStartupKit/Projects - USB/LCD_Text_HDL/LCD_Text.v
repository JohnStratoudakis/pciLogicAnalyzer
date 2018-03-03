// Sample code to drive a Text LCD module from Dragon's USB port.
// (c) fpga4fun.com KNJN LLC - 2004, 2005
//
// The USB bus is much faster than the LCD module, 
// so to keep things simple and to avoid overwhelming the LCD module, 
// we send USB bytes one by one (or two bytes for commands since
// we use a "0" prefix). 
//
// Refer to 
// http://www.fpga4fun.com/LCDmodule.html
// for more details.

module LCD_Text
(
  CLK_USB, USB_FWRn, USB_D, 
  LCD_RS, LCD_RW, LCD_E, LCD_DB
);

input CLK_USB, USB_FWRn;

// nothing useful to send here, so USB_D is used as an input only
input [7:0] USB_D;

output LCD_RS, LCD_RW, LCD_E;
output [7:0] LCD_DB;

////////////////////////////////////////////////////////////////////
// get the USB data
// remember that USB_FWRn is active low
reg [7:0] LCD_USBDATA;
always @(posedge CLK_USB) if(~USB_FWRn) LCD_USBDATA <= USB_D;

////////////////////////////////////////////////////////////////////
// now drive the LCD
// the code looks very similar to the RS-232 LCD module code
// we get a "0" prefix for LCD commands
assign LCD_RW = 0;
assign LCD_DB = LCD_USBDATA;

reg RxDUSB_data_ready;
always @(posedge CLK_USB) RxDUSB_data_ready <= ~USB_FWRn;

wire Received_Escape = RxDUSB_data_ready & (LCD_USBDATA==0);
wire Received_Data = RxDUSB_data_ready & (LCD_USBDATA!=0);

reg [2:0] count;
always @(posedge CLK_USB) if(Received_Data | (count!=0)) count <= count + 1;

// activate LCD_E for 6 clocks, so at 24MHz, that's 6x41.6ns=250ns
reg LCD_E;
always @(posedge CLK_USB)
if(LCD_E==0)
  LCD_E <= Received_Data;
else
  LCD_E <= (count!=6);

reg LCD_instruction;
always @(posedge CLK_USB)
if(LCD_instruction==0)
  LCD_instruction <= Received_Escape;
else
  LCD_instruction <= (count!=7);

assign LCD_RS = ~LCD_instruction;

endmodule
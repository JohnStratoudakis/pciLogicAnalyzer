// PCI example design for the Dragon board - fpga4fun.com
// (c) fpga4fun.com KNJN LLC - 2004, 2005

module PCI_IORAM_LCD
(
  // PCI pins
  PCI_CLK, PCI_RSTn, PCI_FRAMEn, PCI_AD, PCI_CBE, PCI_IRDYn, PCI_TRDYn, PCI_DEVSELn,

  // other pins
  LCD_RS, LCD_RW, LCD_E, LCD_DB,

  // unused PCI pins
  PCI_IDSEL, PCI_PAR, PCI_GNTn, PCI_LOCKn, PCI_PERRn, PCI_REQn, PCI_SERRn, PCI_STOPn
);

parameter IO_address = 32'h00000200;
parameter PCI_CBECD_IORead  = 4'b0010;
parameter PCI_CBECD_IOWrite = 4'b0011;

input PCI_CLK, PCI_RSTn, PCI_FRAMEn, PCI_IRDYn;
inout [31:0] PCI_AD;
input [3:0] PCI_CBE;
output PCI_TRDYn, PCI_DEVSELn;

output LCD_RS, LCD_RW, LCD_E;
output [7:0] LCD_DB;

// unused pins still need to be present because they are assigned the "PCI33_5" IO standard in the UCF file
input PCI_IDSEL, PCI_PAR, PCI_GNTn, PCI_LOCKn, PCI_PERRn, PCI_REQn, PCI_SERRn, PCI_STOPn;  // unused pins
// dummy equation so that unused pins are not synthesized away
wire Dummy1 = PCI_IDSEL | PCI_PAR | PCI_GNTn | PCI_LOCKn | PCI_PERRn | PCI_REQn | PCI_SERRn | PCI_STOPn;  // always equals 1

////////////////////////////////////////////////////////////////////////////////
reg PCI_Transaction;

wire PCI_TransactionStart = ~PCI_Transaction & ~PCI_FRAMEn;
wire PCI_TransactionEnd = PCI_Transaction & PCI_FRAMEn & PCI_IRDYn;

always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_Transaction <= 0;
else
case(PCI_Transaction)
  1'b0: PCI_Transaction <= PCI_TransactionStart;
  1'b1: PCI_Transaction <= ~PCI_TransactionEnd;
endcase

// We respond only to IO reads/writes, 32-bits aligned
wire PCI_Targeted = PCI_TransactionStart & (PCI_AD[/*31*/15:6]==(IO_address>>6)) /*& (PCI_AD[1:0]==0)*/& ((PCI_CBE==PCI_CBECD_IORead) | (PCI_CBE==PCI_CBECD_IOWrite));

// When a transaction starts, the address is available for us to register
// We just need a 4 bits address here
reg [3:0] PCI_TransactionAddr;
always @(posedge PCI_CLK) if(PCI_TransactionStart) PCI_TransactionAddr <= PCI_AD[5:2];

wire PCI_LastDataTransfer = PCI_FRAMEn & ~PCI_IRDYn & ~PCI_TRDYn;

// Is it a read or a write?
reg PCI_Transaction_Read_nWrite;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_Transaction_Read_nWrite <= 0;
else
if(~PCI_Transaction & PCI_Targeted) PCI_Transaction_Read_nWrite <= ~PCI_CBE[0];

// Should we claim the transaction?
reg PCI_DevSelOE;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_DevSelOE <= 0;
else
case(PCI_Transaction)
  1'b0: PCI_DevSelOE <= PCI_Targeted;
  1'b1: if(PCI_TransactionEnd) PCI_DevSelOE <= 1'b0;
endcase

// PCI_DEVSELn should be asserted up to the last data transfer
reg PCI_DevSel;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_DevSel <= 0;
else
case(PCI_Transaction)
  1'b0: PCI_DevSel <= PCI_Targeted;
  1'b1: PCI_DevSel <= PCI_DevSel & ~PCI_LastDataTransfer;
endcase

// PCI_TRDYn is asserted during the whole PCI_Transaction because we don't need wait-states
// For read transaction, delay by one clock to allow for the turnaround-cycle
reg PCI_TargetReady;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_TargetReady <= 0;
else
case(PCI_Transaction)
  1'b0: PCI_TargetReady <= PCI_Targeted & PCI_CBE[0];  // active now on write, next cycle on reads
  1'b1: PCI_TargetReady <= PCI_DevSel & ~PCI_LastDataTransfer;
endcase

// Drive the AD bus on reads only, and allow for the turnaround cycle
reg PCI_AD_OE;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_AD_OE <= 0;
else
PCI_AD_OE <= PCI_DevSel & PCI_Transaction_Read_nWrite & ~PCI_LastDataTransfer;

// Claim the PCI_Transaction
assign PCI_DEVSELn = PCI_DevSelOE ? ~PCI_DevSel : 1'bZ;
assign PCI_TRDYn = PCI_DevSelOE ? ~PCI_TargetReady : 1'bZ;

////////////////////////////////////////////////////////////////////////////////
wire PCI_DataTransferWrite = PCI_DevSel & ~PCI_Transaction_Read_nWrite & ~PCI_IRDYn & ~PCI_TRDYn;

// Instantiate the RAM
// We use Xilinx's synthesis here (XST), which supports automatic RAM recognition
// The following code creates a distributed RAM, but a blockram could also be used (we have 2 clock cycles to get the data out)
reg [31:0] RAM [15:0];
always @(posedge PCI_CLK) if(PCI_DataTransferWrite) RAM[PCI_TransactionAddr] <= PCI_AD;

// now we can drive the PCI_AD bus
assign PCI_AD = PCI_AD_OE ? RAM[PCI_TransactionAddr] : 32'hZZZZZZZZ;

////////////////////////////////////////////////////////////////////////////////
reg [31:0] PCI_data; always @(posedge PCI_CLK) if(PCI_DataTransferWrite) PCI_data <= PCI_AD;

assign LCD_RW = ~Dummy1;
assign LCD_DB = PCI_data[7:0];

reg data_ready;
always @(posedge PCI_CLK) data_ready <= PCI_DataTransferWrite;

reg [2:0] count;
always @(posedge PCI_CLK) if(data_ready | (count!=0)) count <= count + 1;

reg LCD_E;
always @(posedge PCI_CLK)
if(LCD_E==0)
  LCD_E <= data_ready;
else
  LCD_E <= (count!=6);

assign LCD_RS = PCI_TransactionAddr[0];

endmodule

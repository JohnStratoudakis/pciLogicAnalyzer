// PCI_LogicAnalyzer design
// (c) fpga4fun.com KNJN LLC - 2004, 2005
//
// We implement a regular "PCI_IORAM" design
// We add some logic to store the PCI bus signals into blockrams for 256 clocks

module PCI_LogicAnalyzer
(
  // PCI pins
  PCI_CLK, PCI_RSTn, PCI_FRAMEn, PCI_AD, PCI_CBE, PCI_IRDYn, PCI_TRDYn, PCI_DEVSELn,
  PCI_IDSEL, PCI_PAR, PCI_GNTn, PCI_LOCKn, PCI_PERRn, PCI_REQn, PCI_SERRn, PCI_STOPn,

  // USB
  CLK24, /*USB_FWRn, */USB_FRDn, USB_D, 

  // other pins
  LED, LED2
);

parameter IO_address = 32'h00000200;
parameter PCI_CBECD_IORead  = 4'b0010;
parameter PCI_CBECD_IOWrite = 4'b0011;
parameter PCI_CBECD_MEMRead  = 4'b0110;
parameter PCI_CBECD_MEMWrite = 4'b0111;
parameter PCI_CBECD_CSRead  = 4'b1010;
parameter PCI_CBECD_CSWrite = 4'b1011;

input PCI_CLK, PCI_RSTn, PCI_FRAMEn, PCI_IRDYn;
inout [31:0] PCI_AD;
input [3:0] PCI_CBE;
output PCI_TRDYn, PCI_DEVSELn;
input PCI_IDSEL, PCI_PAR, PCI_GNTn, PCI_LOCKn, PCI_PERRn, PCI_REQn, PCI_SERRn, PCI_STOPn;

input CLK24, USB_FRDn;//, USB_FWRn;
inout [7:0] USB_D;

output LED, LED2;

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

// Stop is used to "disconnect with data" (avoid burst)
reg PCI_Stop;
always @(posedge PCI_CLK or negedge PCI_RSTn)
if(~PCI_RSTn) PCI_Stop <= 0;
else
case(PCI_Transaction)
  1'b0: PCI_Stop <= PCI_Targeted & PCI_CBE[0];  // active now on write, next cycle on reads
  1'b1: PCI_Stop <= PCI_DevSel & ~PCI_FRAMEn;
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
//assign PCI_STOPn = PCI_DevSelOE ? ~PCI_Stop : 1'bZ;

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

wire LED = PCI_data[0];
wire LED2 = PCI_data[1];

////////////////////////////////////////////////////////////////////////////////
// Logic analyzer part
//
reg [47:0] RAM_LA [255:0];  // acquisition RAM
reg [7:0] addra;  // we need an 8 bits address (store the PCI bus for 256 clocks)

// trigger when PCI_Targeted is asserted
reg acquisition;
always @(posedge PCI_CLK) if(&addra) acquisition <= 1'b0; else if(PCI_Targeted) acquisition <= 1'b1;

// We capture the following:
// [31:0] PCI_AD  // 32
// [3:0] PCI_CBE  //  4
// PCI_FRAMEn, PCI_IRDYn  //  2
// PCI_TRDYn, PCI_DEVSELn //  2
// PCI_IDSEL, PCI_PAR, PCI_GNTn, PCI_LOCKn, PCI_PERRn, PCI_REQn, PCI_SERRn, PCI_STOPn  // 8
// Total: 48
wire [47:0] disr = {
  PCI_AD, 
  PCI_CBE, PCI_IRDYn, PCI_TRDYn, PCI_FRAMEn, PCI_DEVSELn, 
  PCI_IDSEL, PCI_PAR, PCI_GNTn, PCI_LOCKn, PCI_PERRn, PCI_REQn, PCI_SERRn, PCI_STOPn};

// Use 48 shift-registers to delay the 48 signals and get pre-trigger acquisition
// But the following doesn't work with XST:
//  reg [47:0] SRDR16 [15:0];  always @(posedge PCI_CLK) {SRDR16} <= {SRDR16[15:1], disr};
// so we use that instead to delay the signals:
wire [47:0] dosr;
SR16_8 SR0(.d(disr[ 7: 0]), .q(dosr[ 7: 0]), .clk(PCI_CLK));
SR16_8 SR1(.d(disr[15: 8]), .q(dosr[15: 8]), .clk(PCI_CLK));
SR16_8 SR2(.d(disr[23:16]), .q(dosr[23:16]), .clk(PCI_CLK));
SR16_8 SR3(.d(disr[31:24]), .q(dosr[31:24]), .clk(PCI_CLK));
SR16_8 SR4(.d(disr[39:32]), .q(dosr[39:32]), .clk(PCI_CLK));
SR16_8 SR5(.d(disr[47:40]), .q(dosr[47:40]), .clk(PCI_CLK));

// write to the RAM
always @(posedge PCI_CLK) if(acquisition) RAM_LA[addra] <= dosr;
always @(posedge PCI_CLK) if(acquisition) addra <= addra + 1;

// read the RAM from the USB
reg [10:0] addrb;  always @(posedge CLK24) if(~USB_FRDn) addrb <= addrb + 1;
reg [7:0] addr_regb;  always @(posedge CLK24) addr_regb <= addrb[10:3];
wire [47:0] dob = RAM_LA[addr_regb];

reg [7:0] USB_Data;
always @(posedge CLK24)
case(addrb[2:0])
  3'h0: USB_Data <= dob[ 7: 0];
  3'h1: USB_Data <= dob[15: 8];
  3'h2: USB_Data <= dob[23:16];
  3'h3: USB_Data <= dob[31:24];
  3'h4: USB_Data <= dob[39:32];
  3'h5: USB_Data <= dob[47:40];
  3'h6: USB_Data <= 8'h01;
  3'h7: USB_Data <= 8'h02;
endcase

assign USB_D = ~USB_FRDn ? USB_Data : 8'hZZ;

endmodule

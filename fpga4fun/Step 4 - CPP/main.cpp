
#include <iostream>
#include "NiFpga_FPGATopLevel.h"
#include "ReadFromDragon.h"

int main(void)
{
//	if (TestUSBConnection() == false) return (0);

	//"..\\PCI_LA.pciacq.FIFO-Write-100-103"
	NiFpga_Status status = NiFpga_Initialize();
	ReadFromDragon("PCI_LA.pciacq");
	if (NiFpga_IsError(status)) {
		std::cout << "\nError calling NiFpga_Initialize: " << status;
		return status;
	}

	NiFpga_Session session;
	status = NiFpga_Open(NiFpga_FPGATopLevel_Bitfile, NiFpga_FPGATopLevel_Signature, "RIO0", 0, &session);
	
	if (NiFpga_IsError(status)) {
		std::cout << "\nError calling NiFpga_Open: " << status;
		return status;
	}

	// Write 1 value
	uint32_t value_in = 6;
	status = NiFpga_WriteU32(session, NiFpga_FPGATopLevel_ControlU32_U32In, value_in);
	if (NiFpga_IsError(status)) {
		std::cout << "\nError calling NiFpga_Open: " << status;
		return status;
	}

	// Read 1 value
	uint32_t value_times_two_out = 0, value_plus_plus_out = 0;
	status = NiFpga_ReadU32(session, NiFpga_FPGATopLevel_IndicatorU32_U32TimesTwo, &value_times_two_out);
	
	if (NiFpga_IsError(status)) {
		std::cout << "\nError calling NiFpga_Open: " << status;
		return status;
	} else {
		std::cout << "\nRead (times 2): " << value_times_two_out;
	}

	status = NiFpga_ReadU32(session, NiFpga_FPGATopLevel_IndicatorU32_U32PP, &value_plus_plus_out);
	if (NiFpga_IsError(status)) {
		std::cout << "\nError calling NiFpga_Open: " << status;
		return status;
	} else {
		std::cout << "\nRead (plus 1): " << value_plus_plus_out;
	}

	NiFpga_Close(session, 0);
	if (NiFpga_IsError(status)) {
		std::cout << "\nError calling NiFpga_Close: " << status;
		return status;
	}

	return (0);
}

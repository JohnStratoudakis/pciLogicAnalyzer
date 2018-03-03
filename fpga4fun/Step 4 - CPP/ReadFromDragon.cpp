// Simple application that reads 16KBytes of data from Dragon's USB port
// and saves the data into a "PCI_LA.pciacq" file.
// (c) fpga4fun.com KNJN LLC - 2004, 2005

#include <windows.h>
#include <assert.h>
#include <tchar.h>
#include <stdio.h>
#include "ReadFromDragon.h"

HANDLE DragonDeviceHandle;

bool TestUSBConnection()
{
	ReadFromDragon("test-file.pcidaq");
	return true;
}

void USB_Open()
{
	DragonDeviceHandle = CreateFile(_T("\\\\.\\DRAGON_USB-0"), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	assert(DragonDeviceHandle!=INVALID_HANDLE_VALUE);
}

void USB_Close()
{
	CloseHandle(DragonDeviceHandle);
}

void USB_BulkWrite(ULONG pipe, void* buffer, WORD buffersize)
{
	DWORD nBytes;
	//int nBytes;
	//LPDWORD pnByes = &nBytes;
	DeviceIoControl(DragonDeviceHandle, 0x222051, &pipe, sizeof(pipe), buffer, buffersize, &nBytes, NULL);
	assert(nBytes=buffersize);	// make sure everything was sent
}

void USB_BulkRead(ULONG pipe, void* buffer, WORD buffersize)
{
	DWORD nBytes;
	DeviceIoControl(DragonDeviceHandle, 0x22204E, &pipe, sizeof(pipe), buffer, buffersize, &nBytes, NULL);
	assert(nBytes=buffersize);	// make sure everything was read
}

void USB_Write(void* buffer, WORD buffersize)
{
	USB_BulkWrite(2, buffer, buffersize);
}

void USB_Read(void* buffer, WORD buffersize)
{
	if(buffersize==0) return;
	USB_BulkWrite(6, &buffersize, 2);
	USB_BulkRead(3, buffer, buffersize);
}

void ReadFromDragon(char *output_filename)
{
	char buf[64*256];  // 16KBytes
	FILE *F;

	// read the whole 16KBytes in one shot
	USB_Open();
	USB_Read(buf, sizeof(buf));
	USB_Close();

	// save the buffer into a file
	F = fopen(output_filename, "wb");
	fwrite(buf, 1, sizeof(buf), F);
	fclose(F);
}
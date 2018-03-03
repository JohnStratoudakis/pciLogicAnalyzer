// Win32 PCI access example application for the Dragon board
// (c) fpga4fun.com KNJN LLC - 2004
//
// This example uses the DragonPCI WDM driver to communicate with Dragon

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <winioctl.h>
#include "ioctl.h"

HANDLE hndFile;

DWORD ReadFromDragon(DWORD Addr)
{
	DWORD IoctlResult, ReturnedLength;
	DWORD buf;

	buf = Addr;
	IoctlResult = DeviceIoControl( hndFile, IOCTL_READ, &buf, sizeof(buf), &buf, sizeof(buf), &ReturnedLength, NULL );

	// Everything went smoothly?
	if(!IoctlResult) { printf("IOCTL_READ failed with code %ld\n", GetLastError() ); exit(1); }
	if(ReturnedLength!=4) { printf("Unexpected data returned from IOCTL_READ"); exit(1); }

	return buf;
}

void WriteToDragon(DWORD Addr, DWORD Data)
{
	DWORD IoctlResult, ReturnedLength;
	DWORD buf[2];

//	printf("Writing 0x%08X at address 0x%X\n", Data, Addr);
	buf[0] = Addr;
	buf[1] = Data;
	IoctlResult = DeviceIoControl( hndFile, IOCTL_WRITE, &buf, sizeof(buf), NULL, 0, &ReturnedLength, NULL );

	// everything went smoothly?
	if(!IoctlResult) { printf("IOCTL_WRITE failed with code %ld\n", GetLastError() ); exit(1); }
	if(ReturnedLength!=0) { printf("Unexpected data returned from IOCTL_WRITE"); exit(1); }
}

void ReadDragonAll()
{
	int i;

	// read all 16 locations from Dragon
	for(i=0; i<16; i++)	printf("%08X  ", ReadFromDragon(i));
	printf("\n");
	Sleep(300);
}

void main()
{
	int i;
	hndFile = CreateFile("\\\\.\\DragonPCI", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(hndFile==INVALID_HANDLE_VALUE) { printf("Unable to open the DragonPCI device.\n"); exit(1); }

	// start fresh
	for(i=0; i<16; i++) WriteToDragon(i, 0);

	WriteToDragon(00, 0x11111111);   ReadDragonAll();
	WriteToDragon(00, 0x12345678);   ReadDragonAll();
	WriteToDragon(13, 0x87654321);   ReadDragonAll();
	WriteToDragon(14, 0x45678912);   ReadDragonAll();

	printf("Press a key to exit...");
	while(!kbhit())
	{
		WriteToDragon(0, i++ & 3);
		Sleep(30);
	}

	CloseHandle(hndFile);
}

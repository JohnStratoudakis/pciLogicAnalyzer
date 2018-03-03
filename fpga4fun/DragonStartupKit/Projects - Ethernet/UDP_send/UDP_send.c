// Ethernet 10BASE-T demo code
// (c) fpga4fun.com KNJN LLC - 2004, 2005

// This design provides an example of UDP transmission
// Dont's forget to:
//  1. set the right IP below
//  2. set the ARP entry in Windows (see Dragon's documentation)

#include <winsock.h>
#include <stdio.h>
#include <conio.h>
#pragma comment (lib, "wsock32.lib")

char* szUDPAddress = "192.168.0.44";	// Dragon's IP
u_short UDPPort = 1024;

SOCKET s;
SOCKADDR_IN addr;

void OpenUDPsocket()
{
	WSADATA wsda;
	WSAStartup(MAKEWORD(1,1), &wsda);
	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(UDPPort);
	addr.sin_addr.s_addr = inet_addr(szUDPAddress);
}

void CloseUDPsocket()
{
	closesocket(s);
	WSACleanup();
}

void SendUDPmessage(char* message, int messageLength)
{
	int ret = sendto(s, message, messageLength, 0, (struct sockaddr *) &addr, sizeof(addr));
	if(ret == SOCKET_ERROR) printf("Failed to send, error %d\n", WSAGetLastError());
}

int main(int argc, int **argv)
{
	int c;
	OpenUDPsocket();
	if(s==SOCKET_ERROR) { printf("Failed to create socket, error %d\n", WSAGetLastError()); exit(1); }

	printf("Press ESC to exit, or any other key to send a 1-byte UDP packet...");
	do
	{
		c = getch();
		if(c==27) break;
		SendUDPmessage((char*)&c, 1);
	}
	while(1);

	CloseUDPsocket();
	return 0;
}

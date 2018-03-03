// Ethernet 10BASE-T demo code
// (c) fpga4fun.com KNJN LLC - 2004, 2005

// This design provides an example of UDP reception

#include <winsock.h>
#include <stdio.h>
#include <conio.h>
#pragma comment (lib, "wsock32.lib")

u_short UDPPort = 1024;

SOCKET s;
SOCKADDR_IN addr;

int OpenUDPsocket()
{
	WSADATA wsda;
	WSAStartup(MAKEWORD(1,1), &wsda);
	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(UDPPort);
	addr.sin_addr.s_addr = INADDR_ANY;
	return s;
}

int BindToUDP_Port()
{
	return bind(s, (struct sockaddr *)&addr, sizeof(addr));
}

void CloseUDPsocket()
{
	if(s!=SOCKET_ERROR) closesocket(s);
	WSACleanup();
}

void SendUDPmessage(char* message, int messageLength)
{
	if(sendto(s, message, messageLength, 0, (struct sockaddr *) &addr, sizeof(addr)) == SOCKET_ERROR)
		printf("Failed to send, error %d\n", WSAGetLastError());
}

int main(int argc, int **argv)
{
	int i, ret, iRemoteAddrLen;
	SOCKADDR_IN remote_addr;
	char MessageUDP[512], MessageOut[512*3];
	char hex[16] = "0123456789ABCDEF";

	if(OpenUDPsocket() == SOCKET_ERROR) 
		printf("Failed to create socket, error %d\n", WSAGetLastError());
	else
	if(BindToUDP_Port() == SOCKET_ERROR) 
		printf("Fail to bind to port %d, error %d\n", UDPPort, WSAGetLastError());
	else
	{
		printf("Waiting for packets (press ESC, or Ctrl-C to force exit)...\n");
		do
		{
			iRemoteAddrLen = sizeof(remote_addr);
			ret = recvfrom(s, MessageUDP, sizeof(MessageUDP), 0, (struct sockaddr *) &remote_addr, &iRemoteAddrLen);
			if(ret == SOCKET_ERROR) { printf("Failed to receive, error %d\n", WSAGetLastError()); break; }

			// Since we can't display a binary message, we convert it to a string of printable hexa ASCII digits
			for(i=0; i<ret; i++)
			{
			   MessageOut[i*3+0] = hex[MessageUDP[i] >>   4];
			   MessageOut[i*3+1] = hex[MessageUDP[i] &  0xF];
			   MessageOut[i*3+2] = ' ';
			}
			MessageOut[ret*3-1] = 0;

			printf("Received %d bytes from %s\n", ret, inet_ntoa(remote_addr.sin_addr));
			printf(" %s\n", MessageOut);
		}
		while(!kbhit() || getch()!=27);
	}

	CloseUDPsocket();
	return 0;
}

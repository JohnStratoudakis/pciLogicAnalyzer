//
// DragonPCITest.c - *ix version of the WDM test program ...
// Win32 PCI access example application for the Dragon board
// (c) fpga4fun.com KNJN LLC - 2004
//
// This example uses the DragonPCI Linux driver to communicate with Dragon

//
// How to build DragonPCITest:
//
//	gcc -Wall -o DragonPCITest DragonPCITest.c
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>

uint32_t ReadFromDragon(int fd, uint32_t Addr)
{
	const char *func = "ReadFromDragon";
	off_t offset = Addr * sizeof(uint32_t);
	uint32_t rdat;
	int nr;

	if(lseek(fd, offset, SEEK_SET) < 0) {
		perror("ReadFromDragon seek error");
		exit(1);
	}

	if((nr = read(fd, &rdat, sizeof(rdat))) < 0) {
		perror("ReadFromDragon read error");
		exit(1);
	}

	if(nr != sizeof(rdat))
		fprintf(stderr, "%s: WARNING - read returned %d, expected %d\n",
					func, nr, sizeof(rdat));

	return rdat;
}

void WriteToDragon(int fd, uint32_t Addr, uint32_t Data)
{
	const char *func = "WriteToDragon";
	off_t offset = Addr * sizeof(uint32_t);
	uint32_t wdat = Data;
	int nw;

	if(lseek(fd, offset, SEEK_SET) < 0) {
		fprintf(stderr, "%s: seek error\n", func);
		perror("WriteToDragon seek error");
		exit(1);
	}

	if((nw = write(fd, &wdat, sizeof(wdat))) < 0) {
		perror("WriteToDragon write error");
		exit(1);
	}

	if(nw != sizeof(wdat))
		fprintf(stderr, "%s: WARNING - write returned %d, expected %d\n",
					func, nw, sizeof(wdat));
}

void ReadDragonAll(int fd)
{
	int i;

	// read all 16 locations from Dragon
	for(i=0; i<16; i++)	printf("%08X  ", ReadFromDragon(fd, i));
	printf("\n\n");
	sleep(1);
}

/*
 * Test reading multiple PCI_PnP locations.
 */
void ReadDragonAll2(int fd)
{
	unsigned long buf[16];
	int nr, i;

	if(lseek(fd, 0, SEEK_SET) < 0) {
		perror("ReadDragonAll2 seek error");
		exit(1);
	}

	if((nr = read(fd, buf, sizeof(buf))) < 0) {
		perror("ReadDragonAll2 read error");
		exit(1);
	}

	for(i=0; i<16; i++)	printf("%08lx  ", buf[i]);
	printf("\n\n");
	sleep(1);
}

/*
 * Test writing multiple PCI_PnP locations.
 */
void WriteDragonAll2(int fd, const unsigned long *buf, int count)
{
	int nw;

	if(lseek(fd, 0, SEEK_SET) < 0) {
		perror("WriteDragonAll2 seek error");
		exit(1);
	}

	if((nw = write(fd, buf, count)) < 0) {
		perror("WriteDragonAll2 write error");
		exit(1);
	}
}

#include <sys/select.h>

/*
 * kbhit - keyboard hit look-alike. Only works for <CR> unless in raw mode.
 */
int kbhit(void)
{
	struct timeval tv;
	fd_set fds;

	tv.tv_sec=0;
	tv.tv_usec=0;
	FD_ZERO(&fds);
	FD_SET(0, &fds);

	if(select(1, &fds, NULL, NULL, &tv) < 0)
		return 0;

	return FD_ISSET(0, &fds);
}

int main(int argc, char **argv)
{
	const char *devname = "/dev/DragonPCI";
	const unsigned long buf[16] = {
		0x00000000, 0x11111111, 0x22222222, 0x33333333,
		0x44444444, 0x55555555, 0x66666666, 0x77777777,
		0x88888888, 0x99999999, 0xaaaaaaaa, 0xbbbbbbbb,
		0xcccccccc, 0xdddddddd, 0xeeeeeeee, 0xffffffff,
	};
	int i, fd;

	if((fd = open(devname, O_RDWR)) < 0) {
		fprintf(stderr, "%s open error\n", devname);
		perror("");
		return 1;
	}

	// start fresh
	for(i=0; i<16; i++) WriteToDragon(fd, i, 0);

	WriteToDragon(fd, 00, 0x11111111);   ReadDragonAll(fd);
	WriteToDragon(fd, 00, 0x12345678);   ReadDragonAll(fd);
	WriteToDragon(fd, 13, 0x87654321);   ReadDragonAll(fd);
	WriteToDragon(fd, 14, 0x45678912);   ReadDragonAll(fd);

	WriteDragonAll2(fd, buf, sizeof(buf)); ReadDragonAll2(fd);

	printf("Press return to exit..."); fflush(stdout);
	while(!kbhit())
	{
		WriteToDragon(fd, 0, i++ & 3);
		sleep(1);
	}

	close(fd);

	return 0;
}

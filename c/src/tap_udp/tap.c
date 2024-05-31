// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "tap.h"

int open_tap(char* tap_iface, tap_mode_t mode)
{
	struct ifreq ifr;
	int fd, flags, err;

	switch(mode)
	{
		case tap_readonly:
		flags = O_RDONLY;
		break;

		case tap_writeonly:
		flags = O_WRONLY;
		break;

		default:
		return -1;
	}

	// Flags:
	//   O_RDONLY - read only
	//   O_WRONLY - write only
	//   O_RDWR   - read and write
	//   O_CREAT  - create file if it doesn’t exist
	//   O_EXCL   - prevent creation if it already exists
	if((fd = open("/dev/net/tun", flags)) < 0)
	{
		fprintf(stderr, "Function open failed (name: /dev/net/tun; flags: %d): %s (%d)\n", flags, strerror(errno), errno);
		return fd;
	}

	// Flags:
	//   IFF_TUN   - TUN device (no Ethernet headers)
	//   IFF_TAP   - TAP device
 	//   IFF_NO_PI - Do not provide packet information
	memset(&ifr, 0, sizeof(ifr));
	//strncpy(ifr.ifr_name, tap_iface, IFNAMSIZ);
	memcpy(ifr.ifr_name, tap_iface, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = '\0';
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0)
	{
		fprintf(stderr, "Funtion ioctl failed (flags: TAP device): %s (%d)\n", strerror(errno), errno);
		close(fd);
		return err;
	}
	
	return fd;
}

int read_tap(int tap_fd, size_t capa, unsigned char* buffer, size_t *len)
{
	int ret;
	//memset(buffer, 0, capa);
	if((ret = read(tap_fd, buffer, capa)) < 0)
	{
		fprintf(stderr, "Function read failed: %s (%d)\n", strerror(errno), errno);
		*len = 0;
		return -1;
	}
	*len = (size_t)ret;
	return *len != 0 ? 0 : 1;
}

int write_tap(int tap_fd, unsigned char* buffer, size_t len)
{
	int ret;
	if((ret = write(tap_fd, buffer, len)) < 0)
	{
		fprintf(stderr, "Function write failed: %s (%d)\n", strerror(errno), errno);
		return -1;
	}
	else if(ret != (int)len)
	{
		fprintf(stderr, "Partial bufffer written (%d / %zu bytes)\n", ret, len);
		return -1;
	}
	return 0;
}

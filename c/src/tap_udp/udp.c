// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "udp.h"

int open_udp(struct udp_addr *local)
{
	int fd, val;
	struct sockaddr_in addr;

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		fprintf(stderr, "Function socket failed: %s (%d)\n", strerror(errno), errno);
		return -1;
	}
	val = 1;
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) != 0)
	{
		fprintf(stderr, "Function setsockopt failed: %s (%d)\n", strerror(errno), errno);
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in)); 
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = local->addr;
	addr.sin_port = htons(local->port);

	if(bind(fd, (const struct sockaddr *)(&addr), sizeof(struct sockaddr_in)) != 0)
	{
		fprintf(stderr, "Function bind failed: %s (%d)\n", strerror(errno), errno);
		return -1;
	}
	return fd;
}

int read_udp(int udp_fd, struct udp_addr *remote, size_t capa, unsigned char* buffer, size_t* len)
{
	int ret, flags;
	socklen_t addr_len;
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(struct sockaddr_in)); 
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = remote->addr;
	addr.sin_port = htons(remote->port);

	flags = MSG_WAITALL;
	addr_len = sizeof(struct sockaddr_in);
	if((ret = recvfrom(udp_fd, buffer, capa, flags, (struct sockaddr *)(&addr), &addr_len)) < 0)
	{
		fprintf(stderr, "Function recvfrom failed: %s (%d)\n", strerror(errno), errno);
		return -1;
	}
	if(addr.sin_addr.s_addr != remote->addr || addr.sin_port != htons(remote->port))
	{
		*len = 0;
		return 1;
	}
	*len = ret;

	return 0;
}

int write_udp(int udp_fd, struct udp_addr *remote, unsigned char* buffer, size_t len)
{
	int ret, flags;
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(struct sockaddr_in)); 
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = remote->addr;
	addr.sin_port = htons(remote->port);

	flags = MSG_CONFIRM;
	if((ret = sendto(udp_fd, buffer, len, flags, (const struct sockaddr *)(&addr), sizeof(struct sockaddr_in))) < 0)
	{
		fprintf(stderr, "Function sendto failed: %s (%d)\n", strerror(errno), errno);
		return -1;
	}
	else if(ret != (int)len)
	{
		fprintf(stderr, "Partial bufffer send (%d / %zu bytes)\n", ret, len);
		return -1;
	}
	return 0;
}

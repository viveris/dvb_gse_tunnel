// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <net/if.h>
#include <arpa/inet.h>

#include "utils.h"

int parse_udp_arguments(const char* addr_port, struct udp_addr* addr)
{
	const char *delimiters = ":";
	unsigned long tmp;
	size_t len;
	char *buffer;
	char *end;
	struct in_addr in;

	len = strlen(addr_port) + 1;
	char str[len];

	//if(strncpy(str, addr_port, len) == NULL)
	if(memcpy(str, addr_port, len) == NULL)
	{
		return -1;
	}
	buffer = strtok(str, delimiters);
	if(buffer == NULL)
	{
		return -1;
	}
	if(inet_aton(buffer, &in) == 0)
	{
		return -1;
	}
	addr->addr = in.s_addr;
	buffer = strtok(NULL, delimiters);
	if(buffer == NULL)
	{
		return -1;
	}
	tmp = strtoul(buffer, &end, 10);
	if(strlen(end) > 0 || tmp == 0 || tmp > USHRT_MAX)
	{
		return -1;
	}
	addr->port = (unsigned short)tmp;
	return strtok(NULL, delimiters) == NULL ? 0 : -1;
}

int parse_unsigned_long(const char* str, unsigned long* val)
{
	char *end;

	if(strlen(str) == 1 && str[0] == '0')
	{
		*val = 0;
		return 0;
	}
	*val = strtoul(str, &end, 10);
	return strlen(end) <= 0 && *val != 0 ? 0 : -1;
}

void set_time(unsigned long ms, struct timespec* time)
{
	time->tv_sec = ms / 1000;
	time->tv_nsec = (ms % 1000) * 1000000;
}

unsigned long time_to_long(struct timespec time)
{
	return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

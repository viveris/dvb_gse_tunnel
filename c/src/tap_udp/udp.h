// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

#ifndef __UDP_H__
#define __UDP_H__

#include <stdint.h>

struct udp_addr
{
	uint32_t addr;
	uint16_t port;
};

/**
 * Open an UDP socket
 */
int open_udp(struct udp_addr *local);

/**
 * Read data from an UDP socket
 *
 * Return 0 on success, 1 on timeout, -1 on error
 */
int read_udp(int udp_fd, struct udp_addr *remote, size_t capa, unsigned char* buffer, size_t* len);

/**
 * Write data to an UDP socket
 *
 * Return 0 on success, -1 on error
 */
int write_udp(int udp_fd, struct udp_addr *remote, unsigned char* buffer, size_t len);

#endif

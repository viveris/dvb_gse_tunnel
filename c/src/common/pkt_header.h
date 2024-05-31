// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

#ifndef __PKT_HEADER_H__
#define __PKT_HEADER_H__

#include <stdint.h>

#define MIN_IP_PKT_SIZE  20  // 4 * 5 bytes
#define MIN_MAC_PKT_SIZE 16  // 6 + 6 + 4 bytes

/**
 * Light packet header structure
 */
struct pkt_header
{
	uint64_t src;
	uint64_t dst;
	uint8_t qos;
};

/**
 * Parse the Ethernet header of a packet
 *
 * Return 0 on success, -1 otherwise
 */
int parse_mac_header(unsigned char* buffer, unsigned int len, struct pkt_header* pkth);

/**
 * Write Ethernet address to a buffer
 */
void mac_address_str(uint64_t addr, char* str);

/**
 * Parse the IPv4 header of a packet
 *
 * Return 0 on success, -1 otherwise
 */
int parse_ipv4_header(unsigned char* buffer, unsigned int len, struct pkt_header* pkth);

/**
 * Write IPv4 address to a buffer
 */
void ipv4_address_str(uint32_t addr, char* str);

#endif

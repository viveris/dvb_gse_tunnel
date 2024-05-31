// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "pkt_header.h"

uint64_t compile_mac_address(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4, uint8_t byte5)
{
	uint64_t addr = byte0;
	addr = (addr << 8) + byte1;
	addr = (addr << 8) + byte2;
	addr = (addr << 8) + byte3;
	addr = (addr << 8) + byte4;
	addr = (addr << 8) + byte5;
	return addr;
}

void mac_address_str(uint64_t addr, char* str)
{
	uint8_t b0, b1, b2, b3, b4, b5;
	b5 = addr & 0xff;
	addr = (addr >> 8);
	b4 = addr & 0xff;
	addr = (addr >> 8);
	b3 = addr & 0xff;
	addr = (addr >> 8);
	b2 = addr & 0xff;
	addr = (addr >> 8);
	b1 = addr & 0xff;
	addr = (addr >> 8);
	b0 = addr & 0xff;
	sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", b0, b1, b2, b3, b4, b5);
}

int parse_mac_header(unsigned char* buffer, unsigned int len, struct pkt_header* pkth)
{
	uint16_t type;

	if(len < MIN_MAC_PKT_SIZE)
	{
		fprintf(stderr, "The packet (%u bytes) is smaller than the Ethernet minimal length (%u bytes)\n", len, MIN_MAC_PKT_SIZE);
		return -1;
	}

	// Check packet is the Ethernet type
	type = (buffer[12] << 8) + buffer[13];

	// Extract src, dst and qos to create encap id
	pkth->dst = compile_mac_address(buffer[0], buffer[1], buffer[2], \
			buffer[3], buffer[4], buffer[5]);
	pkth->src = compile_mac_address(buffer[6], buffer[7], buffer[8], \
			buffer[9], buffer[10], buffer[11]);
	if(type == 0x8100) // 802.1Q
	{
		pkth->qos = (buffer[14] >> 1) & 0x7; // PCP
	}
	else if(type == 0x88A8 || type == 0x9100) // 802.1AD (Q in Q)
	{
		pkth->qos = (buffer[16] >> 1) & 0x7; // PCP
	}
	else
	{
		pkth->qos = 0;
	}
	return 0;
}

uint32_t compile_ipv4_address(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3)
{
	return (uint32_t)(byte0 << 24) + (byte1 << 16) + (byte2 << 8) + byte3;
}

void ipv4_address_str(uint32_t addr, char* str)
{
	uint8_t b0, b1, b2, b3;
	b0 = addr >> 24;
	b1 = (addr - (b0 << 24)) >> 16;
	b2 = (addr - (b0 << 24) - (b1 << 16)) >> 8;
	b3 = (addr - (b0 << 24) - (b1 << 16) - (b2 << 8));
	sprintf(str, "%u.%u.%u.%u", b3, b2, b1, b0);
}

int parse_ipv4_header(unsigned char* buffer, unsigned int len, struct pkt_header* pkth)
{
	unsigned int version;

	// Check packet is an IPv4 packet
	version = (buffer[0] & 0xf0) >> 4;
	if(version != 4)
	{
		fprintf(stderr, "The packet is not an IPv4 packet (version %u)\n", version);
		return -1;
	}
	else if(len < MIN_IP_PKT_SIZE)
	{
		fprintf(stderr, "The packet (%u bytes) is smaller than the IPv4 minimal length (%u bytes)\n", len, MIN_IP_PKT_SIZE);
		return -1;
	}

	// Extract src, dst and qos to create encap id
	pkth->src = compile_ipv4_address(buffer[12], buffer[13], buffer[14], buffer[15]);
	pkth->dst = compile_ipv4_address(buffer[16], buffer[17], buffer[18], buffer[19]);
	pkth->qos = buffer[1] >> 2; // DSCP
	return 0;
}

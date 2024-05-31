// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

#ifndef __UTILS_H__
#define __UTILS_H__

#include "udp.h"

/**
 * Parse UDP parameters
 *
 * Return 0 on success, -1 otherwise
 */
int parse_udp_arguments(const char* addr_port, struct udp_addr* addr);

/**
 * Parse non-null unsigned long string
 *
 * Return 0 on success, -1 otherwise
 */
int parse_unsigned_long(const char* str, unsigned long* val);

/**
 * Set time from millisecond value
 */
void set_time(unsigned long ms, struct timespec* time);

/**
 * Get millisecond value from time
 */
unsigned long time_to_long(struct timespec time);

#endif

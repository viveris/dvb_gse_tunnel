// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

#ifndef ___TAP_H__
#define ___TAP_H__

#include <time.h>

typedef enum {
	tap_readonly = 0,
	tap_writeonly = 1
} tap_mode_t;

/**
 * Open an interface TAP
 */
int open_tap(char* tap_iface, tap_mode_t mode);

/**
 * Read data from a TAP interface
 *
 * Return 0 on success, 1 on timeout, -1 on error
 */
int read_tap(int tap_fd, size_t capa, unsigned char* buffer, size_t* len);

/**
 * Write data to a TAP interface
 */
int write_tap(int tap_fd, unsigned char* buffer, size_t len);

#endif

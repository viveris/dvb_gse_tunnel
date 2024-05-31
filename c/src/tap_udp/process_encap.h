// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

#ifndef __PROCESS_ENCAP_H__
#define __PROCESS_ENCAP_H__

#include <sys/time.h>
#include <signal.h>

#include <gse/encap.h>
#include <gse/deencap.h>
#include <gse/constants.h>
#include <gse/status.h>
#include <gse/virtual_fragment.h>
#include <gse/refrag.h>
#include <gse/header_fields.h>

#include "udp.h"

#define MIN_ENCAP_FRAME_SIZE (2 * GSE_MAX_HEADER_LENGTH + 2 * GSE_MAX_TRAILER_LENGTH)

struct process_encap_params
{
	char tap_iface[256];

	struct udp_addr local;
	struct udp_addr remote;

	struct timespec read_timeout;
	struct timespec sched_period;

	int buffer_len;
	int payload_len;
};

/**
 * Run encapsulation process
 *
 * Return 0 on success, 1 on stop, -1 on init error, -2 on loop error
 */
int process_encap(struct process_encap_params* params);

#endif

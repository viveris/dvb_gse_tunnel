// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <net/if.h>

#include "process_encap.h"
#include "utils.h"

// Default values
#define DEFAULT_PAYLOAD_LENGTH 1500  // bytes
#define DEFAULT_BUFFER_LENGTH 8192   // bytes
#define DEFAULT_READ_TIMEOUT 100     // ms

/**
 * Print help message
 */
void usage() {
  fprintf(
      stdout,
      "Usage: satencap -i TAP_IFACE -l LOCAL_ADDR_PORT -r REMOTE_ADDR_PORT\n");
  fprintf(stdout, "                [-p PAYLOAD_LEN]\n");
  fprintf(stdout, "                [-b BUFFER_LEN]\n");
  fprintf(stdout, "                [-t READ_TIMEOUT]\n");
  fprintf(stdout, "                [-h]\n");
  fprintf(stdout, "\n    Required arguments\n");
  fprintf(stdout, "        TAP_IFACE         the TAP interface which receives "
                  "incoming IP packets\n");
  fprintf(stdout, "        LOCAL_ADDR_PORT   the address and port to use as "
                  "source of UDP tunnel (format: \"ADDRESS:PORT\")\n");
  fprintf(stdout, "        REMOTE_ADDR_PORT  the address and port to use as "
                  "destination of UDP tunnel (format: \"ADDRESS:PORT\")\n");
  fprintf(stdout, "\n    Optional arguments\n");
  fprintf(stdout,
          "        PAYLOAD_LEN       the constant size (bytes) of outcoming "
          "payload to the UDP tunnel. Set 0 to send packets of variable "
          "payload length (default: %u)\n",
          DEFAULT_PAYLOAD_LENGTH);
  fprintf(stdout,
          "        BUFFER_LEN        the max size (bytes) of incoming IP "
          "packet (default: %u))\n",
          DEFAULT_BUFFER_LENGTH);
  fprintf(stdout,
          "        READ_TIMEOUT      the timeout (ms) to read incoming IP "
          "packets (default: %u)\n",
          DEFAULT_READ_TIMEOUT);
}

/**
 * Parse encapsulation tunnel parameters
 *
 * Return 0 on success, 1 when help is required, -1 on error
 */
int parse_arguments(int argc, char **argv,
                    struct process_encap_params *params) {
  unsigned int shift = 0;
  const unsigned int error_flag = 1 << ++shift;
  const unsigned int help_flag = 1 << ++shift;

  const unsigned int tap_iface_flag = 1 << ++shift;
  const unsigned int read_timeout_flag = 1 << ++shift;

  const unsigned int local_flag = 1 << ++shift;
  const unsigned int remote_flag = 1 << ++shift;

  const unsigned int sched_period_flag = 1 << ++shift;
  (void)sched_period_flag;

  const unsigned int payload_len_flag = 1 << ++shift;
  const unsigned int frames_count_flag = 1 << ++shift;
  (void)frames_count_flag;

  const unsigned int buffer_len_flag = 1 << ++shift;

  unsigned int flags = 0;
  int c;
  unsigned long val;

  while ((flags & error_flag) == 0 && (flags & help_flag) == 0 &&
         (c = getopt(argc, argv, "hi:l:r:p:c:b:q:s:t:")) != -1) {
    switch (c) {
    case 'h':
      flags |= help_flag;
      break;

    case 'i':
      if (if_nametoindex(optarg) == 0) {
        fprintf(
            stderr,
            "Invalid TAP interface \"%s\": must be an existing TAP interface\n",
            optarg);
        flags |= error_flag;
        break;
      }
      flags |= tap_iface_flag;
      // strncpy(params->tap_iface, optarg, strlen(optarg) + 1);
      memcpy(params->tap_iface, optarg, strlen(optarg) + 1);
      break;

    case 'l':
      if (parse_udp_arguments(optarg, &(params->local)) != 0) {
        fprintf(stderr,
                "Invalid local address and port \"%s\" (format: "
                "\"ADDRESS:PORT\")\n",
                optarg);
        flags |= error_flag;
        break;
      }
      flags |= local_flag;
      break;

    case 'r':
      if (parse_udp_arguments(optarg, &(params->remote)) != 0) {
        fprintf(stderr,
                "Invalid remote address and port \"%s\" (format: "
                "\"ADDRESS:PORT\")\n",
                optarg);
        flags |= error_flag;
        break;
      }
      flags |= remote_flag;
      break;

    case 'p':
      if (parse_unsigned_long(optarg, &val) != 0 || val > UINT_MAX) {
        fprintf(stderr,
                "Invalid payload length \"%s\": the value must be an unsigned "
                "int in bytes\n",
                optarg);
        flags |= error_flag;
        break;
      }
      params->payload_len = val;
      flags |= payload_len_flag;
      break;

    case 'b':
      if (parse_unsigned_long(optarg, &val) != 0 || val > UINT_MAX) {
        fprintf(stderr,
                "Invalid buffer length \"%s\": the value must be an unsigned "
                "long in bytes\n",
                optarg);
        flags |= error_flag;
        break;
      }
      params->buffer_len = val;
      flags |= buffer_len_flag;
      break;

    case 't':
      if (parse_unsigned_long(optarg, &val) != 0) {
        fprintf(stderr,
                "Invalid reading timeout \"%s\": the value must be an unsigned "
                "long in milliseconds\n",
                optarg);
        flags |= error_flag;
        break;
      }
      set_time(val, &(params->read_timeout));
      flags |= read_timeout_flag;
      break;

    case '?':
      fprintf(stderr, "Invalid argument option \"%c\"\n", c);
      flags |= error_flag;
      break;

    default:
      fprintf(stderr, "Invalid arguments\n");
      flags |= error_flag;
      break;
    }
  }
  if ((flags & help_flag) != 0) {
    return 1;
  }
  if ((flags & error_flag) != 0) {
    return -1;
  }

  // Check required arguments
  if ((flags & tap_iface_flag) == 0) {
    fprintf(stderr, "Missing TAP interface argument\n");
    return -1;
  }
  if ((flags & local_flag) == 0) {
    fprintf(stderr, "Missing local address and port argument\n");
    return -1;
  }
  if ((flags & remote_flag) == 0) {
    fprintf(stderr, "Missing remote address and port argument\n");
    return -1;
  }

  // Check optional arguments
  if ((flags & read_timeout_flag) == 0) {
    set_time(DEFAULT_READ_TIMEOUT, &(params->read_timeout));
  }
  if ((flags & payload_len_flag) == 0) {
    params->payload_len = DEFAULT_PAYLOAD_LENGTH;
  }
	if((flags & buffer_len_flag) == 0)
	{
		params->buffer_len = DEFAULT_BUFFER_LENGTH;
	}

  return 0;
}

/**
 * Main function
 *
 * Return 0 on success, -1 on arguments error, -2 on initialization error, -3
 * otherwise
 */
int main(int argc, char **argv) {
  int ret;
  struct process_encap_params params;

  // Parse arguments
  if ((ret = parse_arguments(argc, argv, &params)) != 0) {
    usage();
    return ret > 0 ? 0 : -1;
  }

#ifdef DEBUG
  // Print arguments
  fprintf(stdout, "Configuration\n");
  fprintf(stdout, "  - TAP interface:      \"%s\"\n", params.tap_iface);
  {
    struct in_addr in;
    in.s_addr = params.local.addr;
    fprintf(stdout, "  - local address:      \"%s:%u\"\n", inet_ntoa(in),
            params.local.port);
    in.s_addr = params.remote.addr;
    fprintf(stdout, "  - remote address:     \"%s:%u\"\n", inet_ntoa(in),
            params.remote.port);
  }
  fprintf(stdout, "  - reading timeout:    %lu ms\n",
          time_to_long(params.read_timeout));
  fprintf(stdout, "  - payload length:     %d bytes\n", params.payload_len);
  fprintf(stdout, "  - buffer length:      %d bytes\n", params.buffer_len);
  fprintf(stdout, "\n");
#endif

  // Process encapsulation
  if ((ret = process_encap(&params)) != 0) {
    return -2;
  }
  return 0;
}

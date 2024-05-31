// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

#include <errno.h>
#include <gse/virtual_fragment.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef DEBUG
#include <arpa/inet.h>
#include <net/if.h>

#include "utils.h"
#endif

#include "pkt_header.h"
#include "process_decap.h"
#include "tap.h"
#include "udp.h"

#define CEIL(x, y)                                                             \
  (x / y + (x % y != 0)) // (x, y: Integers) Only works for positive numbers
#define QOS_COUNT 1      // No QoS management applied into libGSE

int check_decap_params(struct process_decap_params *params);

struct decap_ctxt {
  struct timespec timeout;
  sigset_t sigmask;
  struct udp_addr remote;

  int udp_fd;
  int tap_fd;
};
struct decap_ctxt *create_ctxt(struct process_decap_params *params);
void delete_ctxt(struct decap_ctxt *ctxt);

int process_send(struct decap_ctxt *ctxt);

int alive;
void sighandler(__attribute__((unused)) int sig) { alive = 1; }

int process_decap(struct process_decap_params *params) {
  int ret;

  int nfds;
  fd_set fds, readfds;

  sigset_t sigmask;
  struct decap_ctxt *ctxt;

  // Initialization
  if (check_decap_params(params) != 0) {
    return -1;
  }
  if ((ctxt = create_ctxt(params)) == NULL) {
    return -1;
  }

  // Add stop signals handler
  signal(SIGTERM, sighandler);
  signal(SIGINT, sighandler);

  // Mask signals during interface polling
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGTERM);
  sigaddset(&sigmask, SIGINT);

  // Link contexts
  ctxt->sigmask = sigmask;

  FD_ZERO(&fds);
  FD_SET(ctxt->udp_fd, &fds);
  nfds = ctxt->udp_fd + 1;
  alive = 0;

  gse_deencap_t *decap;
  if ((ret = gse_deencap_init(QOS_COUNT, &decap)) != GSE_STATUS_OK) {
    fprintf(stderr, "Deencapsulator initialization failed: %s (%d)\n",
            gse_get_status(ret), ret);
    return -1;
  }

  unsigned char *data_received = (unsigned char *)malloc(
      sizeof(unsigned char) * (GSE_MAX_PACKET_LENGTH + 2));
  size_t len_received;
  size_t len_decapsulated;

  uint8_t label_type;
  uint8_t label[6];
  uint16_t protocol;
  uint16_t gse_length;

  while (alive == 0) {
    gse_vfrag_t *vfrag_pkt = NULL;
    gse_vfrag_t *pdu = NULL;

    readfds = fds;
    ret =
        pselect(nfds, &readfds, NULL, NULL, &(ctxt->timeout), &(ctxt->sigmask));
    if (ret == 0) {
      continue;
    } else if (ret < 0) {
      fprintf(stderr, "[Receiver] Function pselect failed: %s (%d)\n",
              strerror(errno), errno);
      alive = -1;
      break;
    }

    if (FD_ISSET(ctxt->udp_fd, &readfds)) // Incoming packet on UDP socket
    {
      if ((ret = read_udp(ctxt->udp_fd, &(ctxt->remote), params->payload_len,
                          data_received, &(len_received))) < 0) {
        fprintf(stderr, "[Receiver] Packet reading from UDP socket failed\n");
        alive = -1;
        break;
      } else if (0 < ret || len_received == 0) {
        fprintf(stderr, "Invalid destination or empty encapsulation packet\n");
        continue;
      }

      /* Test tap */
      // if((ret = write_tap(ctxt->tap_fd, data_received, len_received)) != 0)
      // {
      // 	fprintf(stderr, "[Sender] TAP sending failed (%d)\n", ret);
      // }
      // continue;
      /*End test tap*/

      len_decapsulated = 0;
      while (len_decapsulated < len_received && alive == 0) {
        ret = gse_create_vfrag_with_data(
            &vfrag_pkt, len_received - len_decapsulated, 0, 0,
            data_received + len_decapsulated, len_received - len_decapsulated);
        if (ret > GSE_STATUS_OK) {
          fprintf(stderr,
                  "Decapsulation fragment initialization failed: %s (%d)\n",
                  gse_get_status(ret), ret);
          fprintf(stderr, "Len decap: %ld, Len received: %ld, Buffer: %d\n",
                  len_decapsulated, len_received, params->buffer_len);
          alive = -1;
        }

        ret = gse_deencap_packet(vfrag_pkt, decap, &label_type, label,
                                 &protocol, &pdu, &gse_length);

        if ((ret > GSE_STATUS_OK) && (ret != GSE_STATUS_PDU_RECEIVED) &&
            (ret != GSE_STATUS_DATA_OVERWRITTEN) &&
            (ret != GSE_STATUS_PADDING_DETECTED)) {
          fprintf(stderr, "Error when de-encapsulating GSE packet: %s (%d)\n",
                  gse_get_status(ret), ret);
        }

        if (ret == GSE_STATUS_INVALID_DATA_LENGTH) {
          fprintf(stderr, "Error, invalid data length: %s (%d)\n",
                  gse_get_status(ret), ret);
        }

        len_decapsulated += gse_length;
        if (ret == GSE_STATUS_DATA_OVERWRITTEN) {
          fprintf(stderr, "PDU incomplete dropped\n");
        }

        if (ret == GSE_STATUS_PADDING_DETECTED) {
          // No more packets, only padding left
          break;
        }

        if (ret == GSE_STATUS_PDU_RECEIVED) {
#ifdef NOPE
          for (int i = 0;
               i < CEIL(gse_get_vfrag_length(pdu), params->buffer_len); ++i) {
            if ((ret = write_tap(ctxt->tap_fd,
                                 gse_get_vfrag_start(pdu) +
                                     (i * params->buffer_len),
                                 params->buffer_len)) != 0) {
              fprintf(stderr, "[Sender] TAP sending failed (%d)\n", ret);
            }
          }
#endif
          write_tap(ctxt->tap_fd, gse_get_vfrag_start(pdu),
                    gse_get_vfrag_length(pdu));
          if ((ret = gse_free_vfrag(&pdu)) != GSE_STATUS_OK) {
            fprintf(stdout, "Decapsulation PDU cleaning failed: %s (%d)\n",
                    gse_get_status(ret), ret);
          }
        }
      }
    }
  }

  // Clean
  delete_ctxt(ctxt);

  return alive >= 0 ? 0 : -2;
}

int check_decap_params(struct process_decap_params *params) {
  if (params->read_timeout.tv_sec == 0 && params->read_timeout.tv_nsec == 0) {
    fprintf(stderr,
            "Invalid reading timeout value: must be strictly positive\n");
    return -1;
  }

  if (params->payload_len < MIN_ENCAP_FRAME_SIZE) {
    fprintf(stderr,
            "Invalid encapsulation frames dimension: at least one frame of %u "
            "bytes must be sent\n",
            MIN_ENCAP_FRAME_SIZE);
    return -1;
  }
  return 0;
}

struct decap_ctxt *create_ctxt(struct process_decap_params *params) {
  struct decap_ctxt *ctxt;

  if ((ctxt = (struct decap_ctxt *)malloc(sizeof(struct decap_ctxt))) == NULL) {
    return NULL;
  }
  memset(ctxt, 0, sizeof(struct decap_ctxt));
  memcpy(&(ctxt->timeout), &(params->read_timeout), sizeof(struct timespec));
  memcpy(&(ctxt->remote), &(params->remote), sizeof(struct udp_addr));

  if ((ctxt->udp_fd = open_udp(&(params->local))) < 0) {
    char l_addr[256];
    ipv4_address_str(params->local.addr, l_addr);
    fprintf(stderr, "UDP tunnel opening on %s:%u failed\n", l_addr,
            params->local.port);
    free(ctxt);
    return NULL;
  }
  if ((ctxt->tap_fd = open_tap((char *)(params->tap_iface), tap_writeonly)) <
      0) {
    fprintf(stderr, "TAP interface %s opening failed\n", params->tap_iface);
    close(ctxt->udp_fd);
    free(ctxt);
    return NULL;
  }

  return ctxt;
}

void delete_ctxt(struct decap_ctxt *ctxt) {
  if (ctxt == NULL) {
    return;
  }
  close(ctxt->udp_fd);
  close(ctxt->tap_fd);
  free(ctxt);
}

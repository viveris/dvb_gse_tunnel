// Copyright 2023, Viveris Technologies
// Distributed under the terms of the MIT License

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef DEBUG
#include <arpa/inet.h>
#include <net/if.h>

#include "utils.h"
#endif

#include "pkt_header.h"
#include "process_encap.h"
#include "tap.h"
#include "udp.h"

#define MAX_FRAG 100 // Maximum fragmentation count for a packet
#define QOS_COUNT 1  // No QoS management applied into libGSE
#define QOS 0
#define PROTOCOL 9029

int check_encap_params(struct process_encap_params *params);

struct encap_recv_ctxt {
  struct timespec timeout;
  sigset_t sigmask;

  int tap_fd;

  struct queue *pkt_q;
};
struct encap_recv_ctxt *create_recv_ctxt(struct process_encap_params *params);
void delete_recv_ctxt(struct encap_recv_ctxt *ctxt);

struct encap_send_ctxt {
  struct timespec timeout;
  sigset_t sigmask;
  struct udp_addr remote;

  int evt_fd;
  int udp_fd;

  struct queue *encap_q;

  int code;
};
struct encap_send_ctxt *create_send_ctxt(struct process_encap_params *params);
void delete_send_ctxt(struct encap_send_ctxt *ctxt);

int alive;
void sighandler(__attribute__((unused)) int sig) { alive = 1; }

int process_encap(struct process_encap_params *params) {
  int ret;

  void *res;
  (void)res;
  int nfds;
  fd_set fds, readfds;

  sigset_t sigmask;
  struct encap_recv_ctxt *ctxt;
  struct encap_send_ctxt *send_ctxt;

  // Initialization
  if (check_encap_params(params) != 0) {
    return -1;
  }
  if ((ctxt = create_recv_ctxt(params)) == NULL) {
    return -1;
  }
  if ((send_ctxt = create_send_ctxt(params)) == NULL) {
    delete_recv_ctxt(ctxt);
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
  send_ctxt->sigmask = sigmask;

  FD_ZERO(&fds);
  FD_SET(ctxt->tap_fd, &fds);
  nfds = ctxt->tap_fd + 1;
  alive = 0;

  size_t len_received;
  gse_encap_t *encap;
  if ((ret = gse_encap_init(QOS_COUNT, MAX_FRAG, &encap)) != GSE_STATUS_OK) {
    fprintf(stderr, "Encapsulator initialization failed: %s (%d)\n",
            gse_get_status(ret), ret);
    return -1;
  }

  gse_vfrag_t *vfrag_pdu = NULL;
  gse_vfrag_t *vfrag_pkt = NULL;
  uint8_t label[6];

  uint64_t counter = 0, err;
  while (alive == 0) {
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

    ret = gse_create_vfrag(&vfrag_pdu, GSE_MAX_PDU_LENGTH,
                           GSE_MAX_HEADER_LENGTH, GSE_MAX_TRAILER_LENGTH);
    if (ret > GSE_STATUS_OK) {
      fprintf(stderr, "Error when creating PDU virtual fragment (%s)\n",
              gse_get_status(ret));
    }

    if (FD_ISSET(ctxt->tap_fd, &readfds)) // Incoming packet on TAP interface
    {
      // if((ret = read_tap(ctxt->tap_fd, params->buffer_len, data_received,
      // &(len_received)))
      // != 0)
      if ((ret = read_tap(ctxt->tap_fd, params->buffer_len,
                          gse_get_vfrag_start(vfrag_pdu), &(len_received))) <
          0) {
#ifdef DEBUG
        fprintf(stdout,
                "Receive nothing from TAP interface\n");
#endif
        gse_free_vfrag(&vfrag_pdu);
        continue; // Ignore
      } else if (len_received == 0) {
#ifdef DEBUG
        fprintf(stdout,
                "Receive empty packet from TAP interface\n");
#endif
        gse_free_vfrag(&vfrag_pdu);
        continue; // Ignore
      }
#ifdef DEBUG
      fprintf(stdout, "[Receiver] Receive packet (%zu bytes)\n", len_received);
#endif

      /* Test TAP */
      // if((ret = write_udp(send_ctxt->udp_fd, &(send_ctxt->remote),
      // data_received, len_received)) != 0) { 	fprintf(stderr, "[Send] Write
      // udp failed\n");
      // }
      // continue;
      /* End test TAP*/

      ret = gse_set_vfrag_length(vfrag_pdu, len_received);
      if (ret > GSE_STATUS_OK) {
        fprintf(stderr, "error when setting fragment length: %s\n",
                gse_get_status(ret));
      }
      if (gse_get_vfrag_length(vfrag_pdu) == 0) {
        fprintf(stderr, "VFRAG empty\n");
      }
      ++counter;
      label[5] = (counter >> 56) & 0xff;
      label[4] = (counter >> 48) & 0xff;
      label[3] = (counter >> 32) & 0xff;
      label[2] = (counter >> 16) & 0xff;
      label[1] = (counter >> 8) & 0xff;
      label[0] = counter & 0xff;
      ret = gse_encap_receive_pdu(vfrag_pdu, encap, label, 0, PROTOCOL, QOS);
      if (ret > GSE_STATUS_OK) {
        fprintf(stderr,
                "VFRAG failed encap: %.2x, vfrag_length: %ld, max_length: %d, "
                "len_received: "
                "%ld\n",
                ret, gse_get_vfrag_length(vfrag_pdu), GSE_MAX_PDU_LENGTH,
                len_received);
        gse_free_vfrag(&vfrag_pdu);
        continue;
      }
      err = 0;
      long desired_len = params->payload_len;
      while (ret != GSE_STATUS_FIFO_EMPTY && err < 5) {
        if (params->payload_len == 0) {
          desired_len = gse_get_vfrag_length(vfrag_pdu) + GSE_MAX_HEADER_LENGTH;
        }
        ret = gse_encap_get_packet(&vfrag_pkt, encap, desired_len, QOS);
        if ((ret > GSE_STATUS_OK) && (ret != GSE_STATUS_FIFO_EMPTY)) {
          fprintf(
              stderr,
              "Error when getting packet from PDU: %s | Demanded length: %ld\n",
              gse_get_status(ret), desired_len);
          err++;
        } else if (ret != GSE_STATUS_FIFO_EMPTY) {
          if (params->payload_len != 0) {
            memset(&gse_get_vfrag_start(
                       vfrag_pkt)[gse_get_vfrag_length(vfrag_pkt)],
                   0, params->payload_len - gse_get_vfrag_length(vfrag_pkt));
            ret = gse_set_vfrag_length(vfrag_pkt, params->payload_len);
            if (ret > GSE_STATUS_OK) {
              fprintf(stderr, "error when setting fragment length: %s\n",
                      gse_get_status(ret));
            }
          }
          if ((ret = write_udp(send_ctxt->udp_fd, &(send_ctxt->remote),
                               gse_get_vfrag_start(vfrag_pkt),
                               gse_get_vfrag_length(vfrag_pkt))) != 0) {
            fprintf(stderr, "[Send] Write udp failed\n");
          }
        }
        gse_free_vfrag(&vfrag_pkt);
      }
    }
  }

  // Clean
  delete_send_ctxt(send_ctxt);
  delete_recv_ctxt(ctxt);

  return alive >= 0 ? 0 : -2;
}

int check_encap_params(struct process_encap_params *params) {
  if (params->read_timeout.tv_sec == 0 && params->read_timeout.tv_nsec == 0) {
    fprintf(stderr,
            "Invalid reading timeout value: must be strictly positive\n");
    return -1;
  }

  if (params->sched_period.tv_sec == 0 && params->sched_period.tv_nsec == 0) {
    fprintf(stderr,
            "Invalid scheduler period value: must be strictly positive\n");
    return -1;
  }
  if (params->payload_len != 0 && params->payload_len < MIN_ENCAP_FRAME_SIZE) {
    fprintf(stderr,
            "Invalid encapsulation frames dimension: at least one frame of %u "
            "bytes must be sent\n",
            MIN_ENCAP_FRAME_SIZE);
    return -1;
  }
  return 0;
}

struct encap_recv_ctxt *create_recv_ctxt(struct process_encap_params *params) {
  struct encap_recv_ctxt *ctxt;

  if ((ctxt = (struct encap_recv_ctxt *)malloc(
           sizeof(struct encap_recv_ctxt))) == NULL) {
    return NULL;
  }
  memset(ctxt, 0, sizeof(struct encap_recv_ctxt));
  memcpy(&(ctxt->timeout), &(params->read_timeout), sizeof(struct timespec));

  if ((ctxt->tap_fd = open_tap((char *)(params->tap_iface), tap_readonly)) <
      0) {
    fprintf(stderr, "TAP interface %s opening failed\n", params->tap_iface);
    free(ctxt);
    return NULL;
  }

  return ctxt;
}

void delete_recv_ctxt(struct encap_recv_ctxt *ctxt) {
  if (ctxt == NULL) {
    return;
  }
  close(ctxt->tap_fd);
  free(ctxt);
}

struct encap_send_ctxt *create_send_ctxt(struct process_encap_params *params) {
  struct encap_send_ctxt *ctxt;

  if ((ctxt = (struct encap_send_ctxt *)malloc(
           sizeof(struct encap_send_ctxt))) == NULL) {
    return NULL;
  }
  memset(ctxt, 0, sizeof(struct encap_send_ctxt));
  memcpy(&(ctxt->timeout), &(params->read_timeout), sizeof(struct timespec));
  memcpy(&(ctxt->remote), &(params->remote), sizeof(struct udp_addr));

  if ((ctxt->evt_fd = eventfd(0, 0)) < 0) {
    fprintf(stderr, "Function eventfd failed: %s (%d)\n", strerror(errno),
            errno);
    free(ctxt);
    return NULL;
  }
  if ((ctxt->udp_fd = open_udp(&(params->local))) < 0) {
    char l_addr[256];
    ipv4_address_str(params->local.addr, l_addr);
    fprintf(stderr, "UDP tunnel opening on %s:%u failed\n", l_addr,
            params->local.port);
    close(ctxt->evt_fd);
    free(ctxt);
    return NULL;
  }

  return ctxt;
}

void delete_send_ctxt(struct encap_send_ctxt *ctxt) {
  if (ctxt == NULL) {
    return;
  }
  close(ctxt->udp_fd);
  close(ctxt->evt_fd);
  free(ctxt);
}

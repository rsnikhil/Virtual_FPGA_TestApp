// Copyright (c) 2026 Rishiyur S. Nikhil. All Rights Reserved

// ****************************************************************
// Implements Virtual FPGA Host-side, layer 2 (multi-queue).
// Note, below, it #include's a generated file containing app-specific
// data structures and functions.

// ================================================================
// Includes from C lib 

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

// ----------------
// Includes from this project

#include "VF_Host_L1.h"
#include "VF_Host_L2.h"

// ****************************************************************
// Utilities

static int l2_verbosity = 1;

static
uint16_t mk2B (uint8_t xh, uint8_t xl)
{
    uint16_t result = xh;
    return ((xh << 8) | xl);
}

// ****************************************************************
// Queue data structure (for sending and receiving)
//   _B suffixes: # of bytes
//   _I suffixes: # of items

typedef struct {
    uint16_t    width_B;
    uint16_t    capacity_tx_I;
    uint16_t    capacity_rx_I;
    uint16_t    size_I;       // current occupancy
    uint16_t    credits_I;    // for receive, this is last-credits-sent
    uint16_t    hd_I;         // offset, in items, into qdata_pB
    uint8_t    *qdata_pB;     // pointer to Bytes of databuffer
} Queue;

static
void print_queue_state (FILE *fp,
			const char  *pre,
			const int    qid,
			const Queue *q,
			const char  *post)
{
    fprintf (fp, "%sQueue[%0d]", pre, qid);
    if (q->size_I == 0)
	fprintf (stdout, " empty");
    else if (q->size_I == q->capacity_rx_I)
	fprintf (stdout, " full");
    else
	fprintf (fp, " size %0d hd %0d", q->size_I, q->hd_I);
    fprintf (fp, " credits %0d%s", q->credits_I, post);
}

// ----------------
// Queue IDs

typedef uint8_t Qid;

// "Virtual" qids for noops, credit-reports
static const Qid QID_NOOP = 0xFF;
static const Qid QID_CRED = 0xFE;

static
void init_queue (const char     *direction,
		 const uint32_t  width_B,
		 const uint16_t  capacity_tx_I,
		 const uint16_t  capacity_rx_I,
		 Queue          *q)
{
    bool is_f2h = (strcmp (direction, "f2h") == 0);
    // Initialize Queue struct
    q->width_B       = width_B;
    q->capacity_tx_I = capacity_tx_I;
    q->capacity_rx_I = capacity_rx_I;
    q->size_I        = 0;    // initially empty
    q->credits_I     = (is_f2h ? capacity_rx_I : 0);
    q->hd_I          = 0;

    // Allocate storage for Queue data
    int n = width_B * (is_f2h ? capacity_rx_I : capacity_tx_I);
    void *pdata = malloc (n);
    if (pdata == NULL) {
        fprintf (stdout,
                 "ERROR: %s: malloc failed for %0d bytes (queue data)\\n",
                 __FUNCTION__, n);
        exit (1);
    }
    q->qdata_pB = pdata;
}

// ----------------
// App-specific queue info

#include "VF_Host_L2_generated.h"

// ****************************************************************
// Start/initialize Virtual FPGA L2 layer
// Return true (ok) or false (fail)

// PUBLIC
bool vf_l2_start (char *hostname, uint16_t port)
{
    init_all_queues ();
    return vf_l1_start (hostname, port);
}

// ****************************************************************
// Dequeue and return an F2H item to the App
// Return 0 queue is empty; no item available
//        1 an item is dequeued

// PUBLIC
int vf_l2_f2h_pop (const uint8_t qid, uint8_t *buf)
{
    if (qid >= f2h_n_queues) {
	fprintf (stdout, "ERROR: %s: attempt to dequeue from queue %0d\n",
		 __FUNCTION__, qid);
	fprintf (stdout, "       But there are only %0d F2H queues\n",
		 f2h_n_queues);
	fprintf (stdout, "       Quitting this program\n");
	exit (1);
    }

    Queue *q = & (f2h_queues [qid]);

    if (l2_verbosity > 1)
	print_queue_state (stdout, "f2h_pop\n    BEFORE f2h ", qid, q, "\n");
    if (q->size_I == 0) {
	if (l2_verbosity > 1)
	    fprintf (stdout, "    ... empty\n");
	return 0;    // empty
    }
    else {
	uint8_t *p = q->qdata_pB + (q->hd_I * q->width_B);
	memcpy (buf, p, q->width_B);
	q->hd_I = (q->hd_I + 1) % (q->capacity_rx_I);
	q->size_I--;
	q->credits_I = q->credits_I + 1;
	if (l2_verbosity > 1)
	    print_queue_state (stdout, "    AFTER f2h ", qid, q, "\n");
	return 1;
    }
}

// ================================================================
// Enqueue an H2F item from the App
// Return 0 no item is enqueued (queue is full)
//        1 an item is enqueued

// PUBLIC
int vf_l2_h2f_enqueue (const uint8_t qid, const uint8_t *buf)
{
    if (qid >= h2f_n_queues) {
	fprintf (stdout, "ERROR: %s: attempt to enqueue to queue %0d\n",
		 __FUNCTION__, qid);
	fprintf (stdout, "       But there are only %0d H2F queues\n",
		 h2f_n_queues);
	fprintf (stdout, "       Quitting this program\n");
	exit (1);
    }

    Queue *q = & (h2f_queues [qid]);

    if (l2_verbosity > 1)
	print_queue_state (stdout, "    vf_l2_h2f_enqueue\n    BEFORE h2f ",
			   qid, q, "\n");
    if (q->size_I == q->capacity_rx_I) {
	return 0;    // full
	if (l2_verbosity > 1)
	    fprintf (stdout, "    ... full\n");
    }

    uint16_t tl_I = (q->hd_I * q->size_I) % q->capacity_rx_I;
    uint8_t *p = q->qdata_pB + (tl_I * q->width_B);
    memcpy (p, buf, q->width_B);
    q->size_I++;
    if (l2_verbosity > 1)
	print_queue_state (stdout, "    AFTER h2f ", qid, q, "\n");
    return 1;
}

// ****************************************************************
// Move queue data and credits

// ----------------
// Moves data for at most one H2F queue,
// and credits for at most one F2H queue.

static
bool send_h2f ()
{
    bool did_some_work = false;
    uint8_t msg_hdr [4];

    // Move H2F queue items: find non-empty H2F queue with non-zero credits
    for (uint16_t qid = 0; qid < h2f_n_queues; qid++) {
	Queue *q = & (h2f_queues [qid]);
	if ((q->size_I == 0) || (q->credits_I == 0))
	    continue;

	// candidate queue found; send items up to size/available credits (n_I)
	uint16_t n_I = ((q->size_I < q->credits_I) ? q->size_I : q->credits_I);
	if (l2_verbosity != 0)
	    fprintf (stdout, "    L2.send_h2f: qid %0d sending %0d items\n",
		     qid, n_I);
	if (l2_verbosity > 1)
	    print_queue_state (stdout, "    BEFORE h2f ", qid, q, "\n");
	msg_hdr [0] = qid;
	msg_hdr [1] = (n_I & 0xFF);
	msg_hdr [2] = ((n_I >> 8) & 0xFF);
	msg_hdr [3] = q->width_B;
	vf_l1_h2f_send (4, & (msg_hdr [0]));
	// Iterate over items because items may wrap-around in buffer
	for (uint16_t j = 0; j < n_I; j++) {
	    uint8_t *p_hd;
	    if (q->width_B != 0) {
		p_hd = & (q->qdata_pB [q->hd_I * q->width_B]);
		vf_l1_h2f_send (q->width_B, p_hd);
	    }
	    q->size_I = q->size_I - 1;
	    q->hd_I   = (q->hd_I + 1) % q->capacity_tx_I;
	    if (l2_verbosity > 1) {
		fprintf (stdout, "       send item %0d:", j);
		if (q->width_B != 0) {
		    for (int k = 0; k < q->width_B; k++)
			fprintf (stdout, " %02x", p_hd[k]);
		}
		fprintf (stdout, "\n");
	    }
	}
	q->credits_I = q->credits_I - n_I;
	if (l2_verbosity > 1)
	    print_queue_state (stdout, "    AFTER h2f ", qid, q, "\n");
	did_some_work = true;
	break;
    }

    // Move F2H credits: find an F2H queue whose credits have
    // increased since last credit-report
    for (uint16_t qid = 0; qid < f2h_n_queues; qid++) {
	Queue *q = & (f2h_queues [qid]);
	if (q->credits_I == 0)
	    continue;

	// candidate queue found; send credit-update
	if (l2_verbosity > 0)
	    fprintf (stdout, "    L2.send_h2f: f2h qid %0d CRED %0d\n",
		     qid, q->credits_I);
	if (l2_verbosity > 1)
	    print_queue_state (stdout, "    BEFORE f2h ", qid, q, "\n");
	msg_hdr [0] = QID_CRED;
	msg_hdr [1] = qid;
	msg_hdr [2] = (q->credits_I & 0xFF);
	msg_hdr [3] = ((q->credits_I >> 8) & 0xFF);
	vf_l1_h2f_send (4, msg_hdr);
	q->credits_I = 0;
	if (l2_verbosity > 1)
	    print_queue_state (stdout, "    AFTER f2h ", qid, q, "\n");
	did_some_work = true;
	break;
    }
    return did_some_work;
}

// ----------------
// Moves data for at most one F2H queue,
// or credits for at most one H2F queue.

static
bool recv_f2h ()
{
    bool did_some_work = false;
    uint8_t buf [4];

    // Receive f2h header
    vf_l1_f2h_recv (4, buf);
    if (l2_verbosity > 1)
	fprintf (stdout, "    L2.recv_f2h: rec'd hdr: %02x %02x %02x %02x\n",
		 buf[0], buf[1], buf[2], buf[3]);

    // Triage based on qid
    uint8_t qid = buf [0];
    if (qid == QID_NOOP) {
	// no op
    }
    else if (qid == QID_CRED) {
	// Update credits
	// qid is followed by: 8'f2h_qid, 16'credit
	Qid    h2f_qid = buf [1];
	Queue *q       = & (h2f_queues [h2f_qid]);

	uint16_t credits_I = mk2B (buf [3], buf [2]);

	if (l2_verbosity != 0)
	    fprintf (stdout,"    L2.recv_f2h: H2F qid %0d CRED %0d\n",
		     qid, credits_I);
	if (l2_verbosity > 1)
	    print_queue_state (stdout, "    BEFORE h2f ", h2f_qid, q, "\n");
	q->credits_I = q->credits_I + credits_I;
	did_some_work = true;
	if (l2_verbosity > 1)
	    print_queue_state (stdout, "    AFTER h2f ", h2f_qid, q, "\n");
    }
    else if (qid < f2h_n_queues) {
	// Move f2h queue items
	// qid is followed by: 16'n_B, items[n_items]
	Queue *q = & (f2h_queues [qid]);
	uint16_t n_I = mk2B (buf [2], buf [1]);
	if (l2_verbosity != 0)
	    fprintf (stdout, "    L2.recv_f2h: qid %0d %0d items\n", qid, n_I);
	if (l2_verbosity > 1)
	    print_queue_state (stdout, "    BEFORE f2h ", qid, q, "\n");

	// Iterate over items to accommodate buffer wraparound
	for (int j = 0; j < n_I; j++) {
	    uint16_t tl_ix_I = (q->hd_I + q->size_I) % q->capacity_rx_I;
	    vf_l1_f2h_recv (q->width_B, & (q->qdata_pB [tl_ix_I * q->width_B]));
	    if (l2_verbosity > 1) {
		fprintf (stdout, "    item data:");
		for (int k = 0; k < q->width_B; k++)
		    fprintf (stdout, " %02x",
			     q->qdata_pB [tl_ix_I * q->width_B +k]);
		fprintf (stdout, "\n");
	    }
	    q->size_I = q->size_I + 1;
	}
	if (l2_verbosity > 1)
	    print_queue_state (stdout, "    AFTER f2h ", qid, q, "\n");
	did_some_work = true;
    }
    else {
	fprintf (stdout, "ERROR: %s: received F2H qid = %0d\n",
		 __FUNCTION__, qid);
	fprintf (stdout, "       But there are only %0d F2H queues\n",
		 f2h_n_queues);
	fprintf (stdout, "       Quitting this program\n");
	exit (1);
    }
    return did_some_work;
}

// ================================================================
// PUBLIC

bool vf_l2_thread_body ()
{
    bool h2f_work = send_h2f ();
    bool f2h_work = recv_f2h ();
    return (h2f_work || f2h_work);
}

// ****************************************************************
// Host_shutdown takes pointer to state returned by Host_init
// Return true (ok) or false (fail)

void vf_l2_finish ()
{
    vf_l1_finish ();
}

// ****************************************************************

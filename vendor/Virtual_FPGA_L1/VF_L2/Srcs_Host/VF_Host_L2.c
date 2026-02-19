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
#include <pthread.h>
#include <errno.h>

// ----------------
// Includes from this project

#include "VF_Host_L1.h"
#include "VF_Host_L2.h"

// ****************************************************************
// Utilities

static int l2_verbosity         = 0;    // for start/finish
static int l2_send_verbosity    = 0;
static int l2_recv_verbosity    = 0;
static int l2_enqueue_verbosity = 0;
static int l2_pop_verbosity     = 0;

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
    uint16_t         width_B;
    uint16_t         capacity_tx_I;
    uint16_t         capacity_rx_I;
    uint16_t         size_I;       // current occupancy
    uint16_t         hd_I;         // offset, in items, into qdata_pB
    uint16_t         credits_I;    // for receive, this is last-credits-sent
    uint8_t         *qdata_pB;     // pointer to Bytes of databuffer
    pthread_mutex_t  mutex;
} Queue;

static
void lock_queue (const char *context, const int qid, Queue *q)
{
    int j = 0;
    while (true) {
	j++;
	int rc = pthread_mutex_trylock (& (q->mutex));
	if (rc == 0) break;
	else if (rc != EBUSY) {
	    fprintf (stdout, "ERROR: %s: %s: pthread_mutex_trylock failed for qid %0d\n",
		     context, __FUNCTION__, qid);
	    perror (NULL);
	    exit (1);
	}
	else if (j >= 0xFFFF) {
	    fprintf (stdout,
		     "ERROR: %s: %s: pthread_mutex_trylock failed: qid %0d attempts %0d\n",
		     context, __FUNCTION__, qid, j);
	    exit (1);
	}
    }
}

static
void unlock_queue (const char *context, const int qid, Queue *q)
{
    int rc = pthread_mutex_unlock (& (q->mutex));
    if (rc != 0) {
	fprintf (stdout, "ERROR: %s: %s: pthread_mutex_unlock failed for %0d\n",
		 context, __FUNCTION__, qid);
	perror (NULL);
	exit (1);
    }
}

static
void print_queue_state (FILE *fp,
			const char  *pre,
			const int    qid,
			const Queue *q,
			const char  *post)
{
    fprintf (fp, "%s[%0d](%0d,%0d)", pre, qid, q->capacity_tx_I, q->capacity_rx_I);
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

// ----------------
// Initialize a queue

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
    int rc = pthread_mutex_init (& (q->mutex), NULL);
    if (rc != 0) {
	perror ("init_queue");
	exit (1);
    }

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
// Decommission a queue

static
void finalize_queue (Queue *q)
{
    int rc = pthread_mutex_destroy (& (q->mutex));
    if (rc != 0) {
	perror ("finalize_queue");
	exit (1);
    }
}

// ----------------
// App-specific queue info

#include "VF_Host_L2_generated.h"

// ****************************************************************

void show_all_queues (FILE *fp)
{
    fprintf (fp, "Queue states: ----------------\n");
    for (int qid = 0; qid < h2f_n_queues; qid++)
	print_queue_state (fp, "H->F:", qid, & (h2f_queues [qid]), "\n");
    for (int qid = 0; qid < f2h_n_queues; qid++)
	print_queue_state (fp, "H<-F:", qid, & (f2h_queues [qid]), "\n");
    fprintf (fp, "----------------\n");
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
    lock_queue (__FUNCTION__, qid, q);

    int result = 0;

    if (q->size_I == 0) {
	if (l2_pop_verbosity > 1)
	    fprintf (stdout, "H<-F[%0d] pop ... empty\n", qid);
	result = 0;    // empty (no pop)
	goto done;
    }

    if (l2_pop_verbosity > 0) {
	fprintf (stdout, "H<-F[%0d] pop\n", qid);
	print_queue_state (stdout, "    BEFORE ", qid, q, "\n");
    }

    uint8_t *p = q->qdata_pB + (q->hd_I * q->width_B);
    memcpy (buf, p, q->width_B);
    q->hd_I = (q->hd_I + 1) % (q->capacity_rx_I);
    q->size_I--;
    q->credits_I = q->credits_I + 1;
    result = 1;    // success (popped)

    if (l2_pop_verbosity > 0)
	print_queue_state (stdout, "    AFTER ", qid, q, "\n");

 done:
    unlock_queue (__FUNCTION__, qid, q);
    return result;
}

// ================================================================
// Enqueue an H2F item from the App
// Return 0 no item is enqueued (queue is full)
//        1 an item is enqueued

// PUBLIC
int vf_l2_h2f_enqueue (const uint8_t qid, const uint8_t *buf)
{
    if (qid >= h2f_n_queues) {
	fprintf (stdout, "Thread (L2 queue maintenance)\n");
	fprintf (stdout, "ERROR: %s: attempt to enqueue to queue %0d\n",
		 __FUNCTION__, qid);
	fprintf (stdout, "       But there are only %0d H2F queues\n",
		 h2f_n_queues);
	fprintf (stdout, "       Quitting this program\n");
	exit (1);
    }

    Queue *q = & (h2f_queues [qid]);
    lock_queue (__FUNCTION__, qid, q);

    int result = 0;

    if (q->size_I == q->capacity_tx_I) {
	if (l2_enqueue_verbosity > 1)
	    fprintf (stdout, "H->F[%0d] enqueue ... full\n", qid);
	result = 0;    // full (no enqueue)
	goto done;
    }

    if (l2_enqueue_verbosity > 0) {
	fprintf (stdout, "H->F[%0d] enqueue\n", qid);
	print_queue_state (stdout, "    BEFORE ", qid, q, "\n");
    }

    uint16_t tl_I = (q->hd_I + q->size_I) % q->capacity_tx_I;
    uint8_t *p = q->qdata_pB + (tl_I * q->width_B);
    memcpy (p, buf, q->width_B);
    q->size_I++;
    result = 1;    // success (enqueued)

    if (l2_enqueue_verbosity > 0)
	print_queue_state (stdout, "    AFTER ", qid, q, "\n");

 done:
    unlock_queue (__FUNCTION__, qid, q);
    return result;
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

    if (l2_send_verbosity > 2)
	fprintf (stdout, "--> SEND (L2 queue maintenance thread)\n");

    // Move H2F queue items: find non-empty H2F queue with non-zero credits
    if (l2_send_verbosity > 2)
	fprintf (stdout, "    Try send F->H ITEMS ...\n");
    for (uint16_t qid_h2f = 0; qid_h2f < h2f_n_queues; qid_h2f++) {
	Queue *q = & (h2f_queues [qid_h2f]);

	lock_queue (__FUNCTION__, qid_h2f, q);

	if ((q->size_I == 0) || (q->credits_I == 0)) {
	    unlock_queue (__FUNCTION__, qid_h2f, q);
	    continue;
	}

	// candidate queue found; send items up to size/available credits (n_I)
	uint16_t n_I = ((q->size_I < q->credits_I) ? q->size_I : q->credits_I);
	if (l2_send_verbosity != 0) {
	    fprintf (stdout, "Thread (L2 queue maintenance)\n");
	    fprintf (stdout, "    send %0d ITEMS for H->F[%0d]\n", n_I, qid_h2f);
	}
	if (l2_send_verbosity > 1)
	    print_queue_state (stdout, "    BEFORE H->F ", qid_h2f, q, "\n");
	msg_hdr [0] = qid_h2f;
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
	    if (l2_send_verbosity > 1) {
		fprintf (stdout, "    item %0d:", j);
		if (q->width_B != 0) {
		    for (int k = 0; k < q->width_B; k++)
			fprintf (stdout, " %02x", p_hd[k]);
		}
		fprintf (stdout, "\n");
	    }
	}
	q->credits_I = q->credits_I - n_I;
	if (l2_send_verbosity > 1)
	    print_queue_state (stdout, "    AFTER H->F ", qid_h2f, q, "\n");
	did_some_work = true;

	unlock_queue (__FUNCTION__, qid_h2f, q);
	break;
    }

    // Move F2H credits: find an F2H queue whose credits have
    // increased since last credit-report
    if (l2_send_verbosity > 2) {
	fprintf (stdout, "Thread (L2 queue maintenance)\n");
	fprintf (stdout, "    Try send F<-H CREDITS ...\n");
    }
    for (uint16_t qid_f2h = 0; qid_f2h < f2h_n_queues; qid_f2h++) {
	Queue *q = & (f2h_queues [qid_f2h]);

	lock_queue (__FUNCTION__, qid_f2h, q);

	if (q->credits_I == 0) {
	    unlock_queue (__FUNCTION__, qid_f2h, q);
	    continue;
	}

	// candidate queue found; send credit-update
	if (l2_send_verbosity > 0) {
	    fprintf (stdout, "Thread (L2 queue maintenance)\n");
	    fprintf (stdout, "    send %0d CREDITS for H<-F[%0d]\n", q->credits_I, qid_f2h);
	}
	if (l2_send_verbosity > 1)
	    print_queue_state (stdout, "    BEFORE H<-F ", qid_f2h, q, "\n");
	msg_hdr [0] = QID_CRED;
	msg_hdr [1] = qid_f2h;
	msg_hdr [2] = (q->credits_I & 0xFF);
	msg_hdr [3] = ((q->credits_I >> 8) & 0xFF);
	vf_l1_h2f_send (4, msg_hdr);
	q->credits_I = 0;
	if (l2_send_verbosity > 1)
	    print_queue_state (stdout, "    AFTER H<-F ", qid_f2h, q, "\n");
	did_some_work = true;

	unlock_queue (__FUNCTION__, qid_f2h, q);
	break;
    }

    if (l2_send_verbosity > 2)
	fprintf (stdout, "<-- SEND (L2 queue maintenance thread)\n");
    return did_some_work;
}

// ----------------
// Moves data for at most one F2H queue,
// or credits for at most one H2F queue.

static
bool recv_f2h ()
{
    if (l2_recv_verbosity > 2)
	fprintf (stdout, "--> RECV (L2 queue maintenance thread)\n");

    bool did_some_work = false;
    uint8_t buf [4];

    // Receive f2h header (if available)
    int rc = vf_l1_f2h_recv_nb (4, buf);
    if (rc == 0)
	// No data available
	return did_some_work;

    if (l2_recv_verbosity > 2) {
	fprintf (stdout, "Thread (L2 queue maintenance)\n");
	fprintf (stdout, "    H<-F: L2 recv HDR: %02x %02x %02x %02x\n",
		 buf[0], buf[1], buf[2], buf[3]);
    }

    // Triage based on qid
    uint8_t qid = buf [0];
    if (qid == QID_NOOP) {
	// no op
    }
    else if (qid == QID_CRED) {
	// Update credits
	// qid is followed by: 8'f2h_qid, 16'credit
	Qid    qid_h2f = buf [1];
	Queue *q       = & (h2f_queues [qid_h2f]);

	lock_queue (__FUNCTION__, qid_h2f, q);

	uint16_t credits_I = mk2B (buf [3], buf [2]);

	if (l2_recv_verbosity != 0) {
	    fprintf (stdout, "Thread (L2 queue maintenance)\n");
	    fprintf (stdout,"    recv %0d CREDITS for H->F[%0d]\n", credits_I, qid_h2f);
	}
	if (l2_recv_verbosity > 1)
	    print_queue_state (stdout, "    BEFORE H->F ", qid_h2f, q, "\n");
	q->credits_I = q->credits_I + credits_I;
	did_some_work = true;
	if (l2_recv_verbosity > 1)
	    print_queue_state (stdout, "    AFTER H->F ", qid_h2f, q, "\n");

	unlock_queue (__FUNCTION__, qid_h2f, q);
    }
    else if (qid < f2h_n_queues) {
	// Move f2h queue items
	// qid is followed by: 16'n_B, items[n_items]
	Queue *q = & (f2h_queues [qid]);

	lock_queue (__FUNCTION__, qid, q);

	uint16_t n_I = mk2B (buf [2], buf [1]);
	if (l2_recv_verbosity != 0) {
	    fprintf (stdout, "Thread (L2 queue maintenance)\n");
	    fprintf (stdout, "    recv %0d ITEMS for H<-F [%0d]\n", n_I, qid);
	}
	if (l2_recv_verbosity > 1)
	    print_queue_state (stdout, "    BEFORE ", qid, q, "\n");

	// Iterate over items to accommodate buffer wraparound
	for (int j = 0; j < n_I; j++) {
	    uint16_t tl_ix_I = (q->hd_I + q->size_I) % q->capacity_rx_I;
	    vf_l1_f2h_recv (q->width_B, & (q->qdata_pB [tl_ix_I * q->width_B]));
	    if (l2_recv_verbosity > 1) {
		fprintf (stdout, "    item data:");
		for (int k = 0; k < q->width_B; k++)
		    fprintf (stdout, " %02x",
			     q->qdata_pB [tl_ix_I * q->width_B +k]);
		fprintf (stdout, "\n");
	    }
	    q->size_I = q->size_I + 1;
	}
	if (l2_recv_verbosity > 1)
	    print_queue_state (stdout, "    AFTER ", qid, q, "\n");
	did_some_work = true;

	unlock_queue (__FUNCTION__, qid, q);
    }
    else {
	fprintf (stdout, "Thread (L2 queue maintenance)\n");
	fprintf (stdout, "ERROR: %s: received F2H qid = %0d\n",
		 __FUNCTION__, qid);
	fprintf (stdout, "       But there are only %0d F2H queues\n",
		 f2h_n_queues);
	fprintf (stdout, "       Quitting this program\n");
	exit (1);
    }

    if (l2_recv_verbosity > 2)
	fprintf (stdout, "<-- RECV (L2 queue maintenance thread)\n");
    return did_some_work;
}

// ****************************************************************
// Start/initialize Virtual FPGA L2 layer
// Return true (ok) or false (fail)

static
pthread_t pthread_for_queue_maintenance;

static
void *queue_maintenance_thread (void *arg_p)
{
    while (true) {
	send_h2f ();
	recv_f2h ();
    }
}

// PUBLIC
void vf_l2_start (char *hostname, uint16_t port)
{
    if (l2_verbosity != 0)
	fprintf (stdout, "%s()->init_all_queues()\n", __FUNCTION__);
    init_all_queues ();

    if (l2_verbosity != 0)
	fprintf (stdout, "%s()->vf_l1_start()\n", __FUNCTION__);
    bool ok = vf_l1_start (hostname, port);
    if (! ok) {
	fprintf (stdout, "ERROR: %s: unable to start L1 layer\n", __FUNCTION__);
	perror (NULL);
	exit (1);
    }

    if (l2_verbosity != 0)
	fprintf (stdout, "%s()->pthread_create()\n", __FUNCTION__);
    int rc = pthread_create (& pthread_for_queue_maintenance,
			     NULL,
			     queue_maintenance_thread,
			     NULL);
    if (rc != 0) {
	fprintf (stdout, "ERROR: %s: unable to create pthread for queue maintenance\n",
		 __FUNCTION__);
	perror (NULL);
	exit (1);
    }
}

// ****************************************************************
// Host_shutdown takes pointer to state returned by Host_init
// Return true (ok) or false (fail)

void vf_l2_finish ()
{
    if (l2_verbosity != 0)
	fprintf (stdout, "%s(): cancel queue-maintenance pthread\n", __FUNCTION__);
    int rc = pthread_cancel (pthread_for_queue_maintenance);
    if (rc != 0) {
	fprintf (stdout, "ERROR: %s: unable to cancel pthread for queue maintenance\n",
		 __FUNCTION__);
	perror (NULL);
	exit (1);
    }

    if (l2_verbosity != 0)
	fprintf (stdout, "%s()->finalize_all_queues()\n", __FUNCTION__);
    finalize_all_queues ();

    vf_l1_finish ();
}

// ****************************************************************

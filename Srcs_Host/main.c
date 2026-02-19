// Copyright (c) 2026 Rishiyur S. Nikhil

// ================================================================
// This contains the top-level main() for the Virtual FPGA Host-side,
// and uses the Virtual FPGA facilities.

// ================================================================
// C lib includes

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

// ----------------
// Project includes

#include "VF_Host_L2.h"

// ================================================================

void enqueue (const int j, const uint64_t data)
{
    uint8_t *buf_p  = (uint8_t *) (& data);

    memcpy (buf_p, & data, 8);
    fprintf (stdout, "App main[%0d]: H->F: data 0x%0" PRIx64 "\n", j, data);
    int k = 0;
    while (true) {
	bool ok = vf_l2_h2f_enqueue (0, buf_p);
	if (ok) break;
	if (k >= 0xFFFF) {
	    fprintf (stdout, "%0d attempts at enqueue\n", k);
	    show_all_queues (stdout);
	    exit (1);
	}
	k++;
    }
    fprintf (stdout, "    ... enqueued\n");
}

void pop (const int j)
{
    uint64_t data;
    uint8_t *buf_p  = (uint8_t *) (& data);

    fprintf (stdout, "App main[%0d]: H<-F ...\n", j);
    int k = 0;
    while (true) {
	data = 0;
	bool ok = vf_l2_f2h_pop (0, buf_p);
	if (ok) break;
	if ((k & 0xFF) == 0xFF) {
	    fprintf (stdout, "%0d attempts at pop\n", k+1);
	    show_all_queues (stdout);
	    exit (1);
	}
	fprintf (stdout, ".");
	fflush (stdout);
	k++;
	usleep (1000);
    }
    fprintf (stdout, "    ... data = 0x%0" PRIx64 "\n", data);
}

// ================================================================

int main (const int argc, const char *argv[])
{
    uint64_t addr = 0x4000;

    fprintf (stdout, "App main: start ...\n");
    vf_l2_start (NULL, 0);

    for (int j = 0; j < 16; j++) {
	enqueue (j, 0x7766554433221100 + j);
	pop (j);
	pop (j);
    }

    fprintf (stdout, "----------------\n");
    fprintf (stdout, "App main: finish ...\n");
    vf_l2_finish ();

    return 0;
}

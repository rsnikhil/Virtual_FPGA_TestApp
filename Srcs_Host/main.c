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

int main (const int argc, const char *argv[])
{
    bool     ok;
    uint64_t addr = 0x4000;
    uint64_t size = 8;
    uint64_t data;
    uint8_t *buf_p  = (uint8_t *) (& data);

    fprintf (stdout, "----------------\n");
    fprintf (stdout, "App main: start ...\n");
    ok = vf_l2_start (NULL, 0);
    if (! ok) {
	fprintf (stdout, "ERROR: in vf_open\n");
	exit (1);
    }

    int step = 0;

    while (true) {
	ok = vf_l2_thread_body ();
	if (! ok) {
	    usleep (1);
	}

	data = 0x7766554433221100;
	memcpy (buf_p, & data, 8);
	if (step == 0) {
	    fprintf (stdout, "----------------\n");
	    fprintf (stdout, "App main: h2f: data 0x%0" PRIx64 "\n", data);
	    ok = vf_l2_h2f_enqueue (0, buf_p);
	    if (ok) {
		fprintf (stdout, "    ... enqueued\n");
		step = 1;
	    }
	    else
		fprintf (stdout, "    queue is full\n");
	}
	else if (step < 3) {
	    fprintf (stdout, "----------------\n");
	    fprintf (stdout, "App main: f2h ...\n");
	    data = 0;
	    ok = vf_l2_f2h_pop (0, buf_p);
	    if (ok) {
		fprintf (stdout, "    ... data = 0x%0" PRIx64 "\n", data);
		step = 2;
	    }
	    else
		fprintf (stdout, "    queue is empty\n");
	}
	else
	    break;
    }
    fprintf (stdout, "----------------\n");
    fprintf (stdout, "App main: finish ...\n");
    vf_l2_finish ();

    return 0;
}

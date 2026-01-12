// Copyright (c) 2026 Rishiyur S. Nikhil. All Rights Reserved

// ****************************************************************
// Implements Virtual FPGA Host-side, layer 1, for simulation.

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

#include "TCP_Client_Lib.h"
#include "VF_Host_L1.h"

// ****************************************************************
// L1 params for TCP connection

// Default host is localhost
static char     DEFAULT_HOSTNAME [] = "127.0.0.1";
static uint16_t DEFAULT_PORT        = 30000;

// ****************************************************************
// Start/initialize Virtual FPGA L1 layer
// Establish TCP connection to FPGA-side on specified hostname and port
//    (NULL hostname implies default host; zero port implies default port)
// Return true (ok) or false (fail)

bool vf_l1_start (char *hostname, uint16_t port)
{
    const int n_secs = 1;

    fprintf (stdout, "%s()\n", __FUNCTION__);

    if (hostname == NULL)
	hostname = & (DEFAULT_HOSTNAME [0]);

    if (port == 0)
	port = DEFAULT_PORT;

    for (int j = 1; j <= 5; j++) {
	if (j != 1) {
	    fprintf (stdout, "  Will sleep and retry.\n");
	    sleep (n_secs);
	    fprintf (stdout, "  Connection retry (attempt #%0d)\n", j);
	}
	uint32_t status = tcp_client_open (hostname, port);
	if (status == TCP_COMMS_STATUS_OK) {
	    fprintf (stdout, "%s: Connected to simulation server\n",
		     __FUNCTION__);
	    return true;
	}
	fprintf (stdout, "%s: unable to connect\n", __FUNCTION__);
	fprintf (stdout, "  Has HW-side sim been started yet?\n");
    }

    fprintf (stdout, "%s: failed several attempts; QUIT\n", __FUNCTION__);
    return false;
}

// ****************************************************************
// Send message to HW-side

void vf_l1_h2f_send (const int n_bytes, const uint8_t *buf)
{
    tcp_client_send (n_bytes, buf);
}

// ****************************************************************
// Receive message from HW-side

void vf_l1_f2h_recv (const int n_bytes, uint8_t *buf)
{
    // Note: in non-TCP transports (e.g., AXI), we may have to send a
    // read-request before receiving this data.
    tcp_client_recv (n_bytes, buf);
}

// ****************************************************************
// Start/initialize Virtual FPGA L1 layer
// Establish TCP connection to FPGA-side on specified hostname and port
//    (NULL hostname implies default host; zero port implies default port)
// Return true (ok) or false (fail)

void vf_l1_finish ()
{
    fprintf (stdout, "%s: closing TCP connection\n", __FUNCTION__);
    tcp_client_close ();
}

// ****************************************************************

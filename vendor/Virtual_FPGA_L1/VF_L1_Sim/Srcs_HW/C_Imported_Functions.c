// Copyright (c) 2026 Rishiyur S. Nikhil.  All Rights Reserved

// ================================================================
// These are functions imported into BSV during Bluesim or Verilog simulation.
// See C_Imports.bsv for the corresponding 'import BDPI' declarations.

// Acknowledgement: portions of TCP code adapted from example ECHOSERV
//   ECHOSERV
//   (c) Paul Griffiths, 1999
//   http://www.paulgriffiths.net/program/c/echoserv.php

// ================================================================
// Includes from C library

// General
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

// For comms polling
#include <poll.h>

// For TCP
#include <sys/socket.h>       //  socket definitions
#include <sys/types.h>        //  socket types
#include <arpa/inet.h>        //  inet (3) funtions
#include <fcntl.h>            // To set non-blocking mode

// ================================================================
// Includes for this project

#include "C_Imported_Functions.h"

// ****************************************************************
// ****************************************************************
// ****************************************************************
// Local debugging controls

static const bool debug_connect    = false;
static const bool debug_disconnect = false;
static const bool debug_recv       = false;
static const bool debug_send       = false;

// ****************************************************************
// Communication with host

// ================================================================
// The socket file descriptor

static int  listen_sockfd = 0;
static int  connected_sockfd = 0;

// ================================================================
// Check if connection is still up, and exit if not

static
void check_connection (int fd, const char *caller)
{
    struct pollfd  x_pollfd;
    x_pollfd.fd      = fd;
    x_pollfd.events  = 0;
    x_pollfd.revents = 0;

    int n = poll (& x_pollfd, 1, 0);

    if ((x_pollfd.revents & POLLHUP) != 0) {
	// Connection has been terminated by remote host (client)
	fprintf (stdout, "%s: terminated by remote host (POLLHUP); exiting\n",
		 __FUNCTION__);
	fprintf (stdout, "    during %s()\n", caller);
	exit (0);
    }
    if ((x_pollfd.revents & POLLERR) != 0) {
	// Connection has been terminated by remote host (client)
	fprintf (stdout, "%s: terminated by remote host (POLLERR); exiting\n",
		 __FUNCTION__);
	fprintf (stdout, "    during %s()\n", caller);
	exit (0);
    }
    if ((x_pollfd.revents & POLLNVAL) != 0) {
	// Connection has been terminated by remote host (client)
	fprintf (stdout, "%s: terminated by remote host (POLLNVAL); exiting\n",
		 __FUNCTION__);
	fprintf (stdout, "    during %s()\n", caller);
	exit (0);
    }
}

// ================================================================
// Start listening on a TCP server socket for a host (client) connection.

static
void  c_host_listen (const uint16_t tcp_port)
{
    struct sockaddr_in  servaddr;             // socket address structure
    struct linger       linger;
  
    if (debug_connect)
	fprintf (stdout,
		 "%s: Listening on tcp port %0d for host connection ...\n",
		 __FUNCTION__, tcp_port);

    // Create the listening socket
    if ( (listen_sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0 ) {
	fprintf (stdout, "ERROR: %s: socket () failed\n", __FUNCTION__);
	exit (1);
    }
  
    // Set linger to 0 (immediate exit on close)
    linger.l_onoff  = 1;
    linger.l_linger = 0;
    setsockopt (listen_sockfd,
		SOL_SOCKET,
		SO_LINGER,
		& linger,
		sizeof (linger));

    // Initialize socket address structure
    memset (& servaddr, 0, sizeof (servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
    servaddr.sin_port        = htons (tcp_port);

    // Bind socket addresss to listening socket
    if (bind (listen_sockfd,
	      (struct sockaddr *) & servaddr,
	      sizeof (servaddr)) < 0) {
	fprintf (stdout, "ERROR: %s: bind () failed\n", __FUNCTION__);
	exit (1);
    }

    // Listen for connection
    if ( listen (listen_sockfd, 1) < 0 ) {
	fprintf (stdout, "ERROR: %s: listen () failed\n", __FUNCTION__);
	exit (1);
    }

    // Set listening socket to non-blocking
    int flags = fcntl (listen_sockfd, F_GETFL, 0);
    if (flags < 0) {
	fprintf (stdout, "ERROR: %s: fcntl (F_GETFL) failed\n", __FUNCTION__);
	exit (1);
    }
    flags = (flags |O_NONBLOCK);
    if (fcntl (listen_sockfd, F_SETFL, flags) < 0) {
	fprintf (stdout, "ERROR: %s: fcntl (F_SETFL, O_NONBLOCK) failed\n",
		 __FUNCTION__);
	exit (1);
    }
}

// ================================================================
// Try to accept a TCP connection from host (remote client)
// Return 1 on success, 0 if no pending connection

static
uint8_t c_host_try_accept ()
{
    connected_sockfd = accept (listen_sockfd, NULL, NULL);
    if ((connected_sockfd < 0)
	&& ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
	// No pending connecction
	return 0;
    }
    else if (connected_sockfd < 0) {
	fprintf (stdout, "ERROR: %s: accept () failed\n", __FUNCTION__);
	exit (1);
    }
    else {
	if (debug_connect)
	    fprintf (stdout, "%s: Connection accepted\n", __FUNCTION__);
	return 1;
    }
}

// ================================================================
// Connect as server to host (client)
// Return ok (true: success, false: fail)

// PUBLIC
uint8_t c_host_connect (const uint16_t tcp_port)
{
    if (debug_connect)
	fprintf (stdout, "  %s: from host connection on port %0d\n",
		 __FUNCTION__, tcp_port);

    // Start listening on given tcp port
    c_host_listen (tcp_port);

    // Wait for, and accept a connection
    while (true) {
	uint8_t ok = c_host_try_accept ();
	if (ok == 1) {
	    if (debug_connect)
		fprintf (stdout, "    ... connected\n");
	    return 1;
	}
	usleep (1000);
    }
}

// ================================================================
// Disconnect as remote host (remote host is client; we are server)
// The 'dummy' arg is not used; is present only to appease some
// Verilog simulators that are finicky about 0-arg functions
// (mistakenly assuming they must be Verilog tasks)

bool c_host_disconnect (uint8_t dummy)
{
    if (debug_disconnect)
	fprintf (stdout, "%s\n", __FUNCTION__);

    check_connection (connected_sockfd, __FUNCTION__);

    // Close the connected socket
    shutdown (connected_sockfd, SHUT_RDWR);
    if (close (connected_sockfd) < 0) {
	fprintf (stdout, "%s: close (connected_sockfd (= %0d)) failed\n",
		 __FUNCTION__, connected_sockfd);
	return false;
    }

    // Close the listening socket
    shutdown (listen_sockfd, SHUT_RDWR);
    if (close (listen_sockfd) < 0) {
	fprintf (stdout, "ERROR: %s: close (listen_sockfd) failed\n",
		 __FUNCTION__);
	return false;
    }
    if (debug_disconnect)
	fprintf (stdout, "    ... disconnected\n");
    return true;
}

// ================================================================
// Size of buffer (num bytes) communicating data between BSV and C
// WARNING: must agree with declaration on BSV side (C_Imports.bsv)

#define SEND_BUF_SIZE_B 64
#define RECV_BUF_SIZE_B 65

// ================================================================
// Receive from host

void c_fromhost_recv (      uint8_t  buf [RECV_BUF_SIZE_B],
		      const uint16_t n_bytes)
{
    check_connection (connected_sockfd, __FUNCTION__);
    
    // Last byte of buf is for AVAIL(1)/UNAVAIL(0) status
    assert (n_bytes < RECV_BUF_SIZE_B);

    // ----------------
    // First, poll to check if any data is available
    int fd = connected_sockfd;

    struct pollfd  x_pollfd;
    x_pollfd.fd      = connected_sockfd;
    x_pollfd.events  = POLLRDNORM;
    x_pollfd.revents = 0;

    int n = poll (& x_pollfd, 1, 0);

    if (n < 0) {
	fprintf (stdout, "ERROR: %s: poll () failed\n", __FUNCTION__);
	exit (1);
    }

    if ((x_pollfd.revents & POLLRDNORM) == 0) {
	buf [RECV_BUF_SIZE_B - 1] = 0;    // UNAVAILABLE
	return;
    }

    // ----------------
    // Data is available; read n_bytes bytes and return
    if (debug_recv)
	fprintf (stdout, "    %s (n_bytes %0d): data are available\n",
		 __FUNCTION__, n_bytes);

    int  n_recd =  0;
    while (n_recd < n_bytes) {
	int n = read (connected_sockfd, & (buf [n_recd]), (n_bytes - n_recd));
	if ((n < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK)) {
	    fprintf (stdout, "ERROR: %s: read () failed on byte 0\n",
		     __FUNCTION__);
	    exit (1);
	}

	n_recd += n;
    }
    buf [RECV_BUF_SIZE_B - 1] = 1;    // AVAILABLE
    if (debug_recv) {
	fprintf (stdout, "    %s:", __FUNCTION__);
	for (int j = 0; j < n_bytes; j++)
	    fprintf (stdout, " %02x", buf [j]);
	fprintf (stdout, "\n");
    }
}

// ================================================================
// Send read-response (n bytes data followed by 1 byte status)
// TODO: repeated calls to send data, up to 512B at a time,
//       then send status.

void c_tohost_send (const uint8_t  buf [SEND_BUF_SIZE_B],
		    const uint16_t n_bytes)
{
    check_connection (connected_sockfd, __FUNCTION__);
    
    assert (n_bytes <= SEND_BUF_SIZE_B);

    if (debug_send) {
	fprintf (stdout, "  %s:", __FUNCTION__);
	for (int j = 0; j < n_bytes; j++)
	    fprintf (stdout, " %02x", buf [j]);
	fprintf (stdout, "\n");
	fflush (stdout);
    }

    uint32_t n_sent = 0;
    while (n_sent < n_bytes) {
	int n = write (connected_sockfd, & (buf [n_sent]), (n_bytes - n_sent));
	if ((n < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK)) {
	    fprintf (stdout,
		     "ERROR: %s: socket-write () failed after %0d bytes\n",
		     __FUNCTION__, n_sent);
	    exit (1);
	}
	else if (n > 0) {
	    n_sent += n;
	}
    }
    fsync (connected_sockfd);
}

// ****************************************************************
// ****************************************************************
// ****************************************************************

// Copyright (c) 2025-2026 Rishiyur S. Nikhil, Inc.  All Rights Reserved

#pragma once

// ================================================================
// See .c file for documentation

// ================================================================

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************
// ****************************************************************
// ****************************************************************
// Communication with host

extern
uint8_t  c_host_connect (const uint16_t tcp_port);

extern
bool c_host_disconnect (uint8_t dummy);

extern
void c_host_recv_req (uint8_t req_buf [17], uint8_t dummy);

extern
void c_host_send_read_rsp (const uint8_t  status,
			   const uint16_t n_bytes,
			   const uint8_t  v[512]);

extern
void c_host_send_write_rsp (const uint8_t status);

// ****************************************************************
// ****************************************************************
// ****************************************************************

#ifdef __cplusplus
}
#endif

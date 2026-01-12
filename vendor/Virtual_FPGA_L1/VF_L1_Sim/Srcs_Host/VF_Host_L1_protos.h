// This file is generated automatically from the file 'VF_Host_L1.c'
//     and contains 'extern' function prototype declarations for its functions.
// In any C source file using these functions, add:
//     #include "VF_Host_L1_protos.h"
// You may also want to create/maintain a file 'VF_Host_L1.h'
//     containing #defines and type declarations.
// ****************************************************************

#pragma once

extern
bool vf_l1_start (char *hostname, uint16_t port);

extern
void vf_l1_h2f_send (const int n_bytes, const uint8_t *buf);

extern
void vf_l1_f2h_recv (const int n_bytes, uint8_t *buf);

extern
void vf_l1_finish ();

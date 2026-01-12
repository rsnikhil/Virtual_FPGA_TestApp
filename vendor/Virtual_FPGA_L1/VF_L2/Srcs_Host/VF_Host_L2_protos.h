// This file is generated automatically from the file 'VF_Host_L2.c'
//     and contains 'extern' function prototype declarations for its functions.
// In any C source file using these functions, add:
//     #include "VF_Host_L2_protos.h"
// You may also want to create/maintain a file 'VF_Host_L2.h'
//     containing #defines and type declarations.
// ****************************************************************

#pragma once

extern
bool vf_l2_start (char *hostname, uint16_t port);

extern
int vf_l2_f2h_pop (const uint8_t qid, uint8_t *buf);

extern
int vf_l2_h2f_enqueue (const uint8_t qid, const uint8_t *buf);

extern
bool vf_l2_thread_body ();

extern
void vf_l2_finish ();

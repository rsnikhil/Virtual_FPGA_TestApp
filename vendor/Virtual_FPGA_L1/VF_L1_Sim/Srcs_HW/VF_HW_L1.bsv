package VF_HW_L1;

// ****************************************************************
// This package is implemented DIFFERENTLY for transport mechanisms
// that varies by platform.

// This package is for HW-side communication to/from the host-side.

// This particular implementation is for TCP transport for HW
// simulation.  It imports C functions to perform the actual
// communication work.

// NOTE: We invoke C functions only inside rules (not methods) for
// predictable ordering.  Logically some of these rules could be
// eliminated by merging them into the methods.

// ****************************************************************
// Imports from BSV lib

import Vector     :: *;
import FIFOF      :: *;
import GetPut_Aux :: *;

// ****************************************************************
// External C functions used here
// See C_Imported_Functions.{h,c} for the corresponding C declarations
// and implementations.

// TODO: this should be configurable at start of run-time
Bit #(16) default_tcp_port = 30000;

// ================================================================
// Connect to remote host on tcp_port (host is client, we are server)
// Returns True (ok) or False (fail).

import "BDPI"
function ActionValue #(Bit #(8))  c_host_connect (Bit #(16)  tcp_port);

// ================================================================
// Disconnect from remote host.
// The 'dummy' arg is not used; is present only to appease some
// Verilog simulators that are finicky about 0-arg functions
// (mistakenly assuming they must be Verilog tasks)

import "BDPI"
function ActionValue #(Bool)  c_host_disconnect (Bit #(8) dummy);

// ================================================================
// Receive from host

typedef 64 H2F_MAX_ITEM_WIDTH_B;    // WARNING! MUST AGREE W. LAYER 2

// H2F_BUF_SIZE_B (below) is size of buffer (# Bytes) communicating
// data from C to BSV, and is 1 + the widest queue item.  The extra
// byte at the end is used by c_fromhost_recv() to indicate whether
// any data was received (1) or not (0), since c_fromhost_recv() is a
// non-blocking receive.

typedef TAdd #(1, H2F_MAX_ITEM_WIDTH_B) H2F_BUF_SIZE_B;

import "BDPI"
function ActionValue #(Vector #(H2F_BUF_SIZE_B, Bit #(8)))
         c_fromhost_recv (Bit #(16) n_bytes);

// ================================================================
// Send to host:
// Size of buffer (bytes) communicating data between BSV and C
// F2H_BUF_SIZE_B must be at least as large as the widest queue item.

typedef 64 F2H_MAX_ITEM_WIDTH_B;    // WARNING! MUST AGREE W. LAYER 2
typedef F2H_MAX_ITEM_WIDTH_B  F2H_BUF_SIZE_B;

import "BDPI"
function Action c_tohost_send (Vector #(F2H_BUF_SIZE_B, Bit #(8)) v,
			       Bit #(16) n_bytes);

// ****************************************************************
// Queue identifiers: 0..254 (8'hFE) in each direction.

typedef Bit #(8) Qid;

Qid qid_CRED = 8'h_FE;    // Reserved for credit-update messages
Qid qid_NOP  = 8'h_FF;    // Reserved for no-op messages

// ****************************************************************
// Misc.

Integer l1_verbosity = 0;

// ****************************************************************
// INTERFACE

interface VF_HW_L1_IFC;
   method Action start ();

   method Action f2h_send (Vector #(F2H_BUF_SIZE_B, Bit #(8)) v,
			   Bit #(16) n_bytes);

   method ActionValue #(Vector #(H2F_BUF_SIZE_B, Bit #(8)))
          h2f_recv (Bit #(16) n_bytes);

   method Action finish ();
endinterface

// ****************************************************************
// MODULE (IMPLENTATION)

typedef enum {STATE_INIT,
	      STATE_CONNECT,
	      STATE_HDR, STATE_DATA,
	      STATE_DISCONNECT,
	      STATE_FINISHED} Run_State
deriving (Bits, Eq, FShow);

(* synthesize *)
module mkVF_HW_L1 (VF_HW_L1_IFC);

   Reg #(Run_State) rg_run_state <- mkReg (STATE_INIT);

   FIFOF #(Tuple2 #(Vector #(F2H_BUF_SIZE_B, Bit #(8)),
		    Bit #(16)))
         f_send <- mkFIFOF;

   // ----------------
   // Start

   rule rl_start (rg_run_state == STATE_CONNECT);
      if (l1_verbosity != 0)
	 $display ("VF_HW_L1.rl_start: using tcp port %0d",
		   default_tcp_port);

      let n <- c_host_connect (default_tcp_port);
      if (n == 0) begin
	 $display ("ERROR: VF_HW_L1.rl_start failed");
	 $finish (1);
      end
      else
	 rg_run_state <= STATE_HDR;
   endrule

   // ----------------
   // Receive incoming messages

   FIFOF #(Vector #(H2F_BUF_SIZE_B, Bit #(8))) f_recv <- mkFIFOF;
   Reg #(Vector #(4, Bit #(8))) rg_hdr <- mkRegU;

   rule rl_recv_hdr (rg_run_state == STATE_HDR);
      let v <- c_fromhost_recv (4);
      if (last (v) == 1) begin
	 f_recv.enq (v);
	 rg_hdr <= take (v);
	 if (l1_verbosity != 0)
	    $display ("VF_HW_L1.rl_recv_hdr [0]..[3]: %02h %02h %02h %02h",
		      v[0], v[1], v[2], v[3]);
	 if (v[0] < qid_CRED)
	    rg_run_state <= STATE_DATA;
      end
   endrule

   rule rl_recv_data (rg_run_state == STATE_DATA);
      Bit #(16) n_I = { rg_hdr[2], rg_hdr[1] };
      Bit #(16) n_B = n_I * zeroExtend (rg_hdr [3]);
      if (l1_verbosity != 0) begin
	 $display ("VF_HW_L1.rl_recv_data: n_bytes = %0d (%0h)", n_B, n_B);
      end
      let v <- c_fromhost_recv (n_B);
      if (last (v) == 1) begin
	 f_recv.enq (v);
	 rg_run_state <= STATE_HDR;
      end
   endrule

   // ----------------
   // Send outgoing messages

   rule rl_send;
      match { .v, .n_bytes } <- pop (f_send);
      c_tohost_send (v, n_bytes);
   endrule

   // ----------------
   // Finish

   rule rl_finish (rg_run_state == STATE_DISCONNECT);
      let ok <- c_host_disconnect (0);
      if (! ok)
	 $display ("ERROR: VF_HW_L1.rl_finish: failed");
      else
	 rg_run_state <= STATE_FINISHED;
   endrule

   // ================================================================
   // INTERFACE

   method start ();
      action
	 rg_run_state <= STATE_CONNECT;
      endaction
   endmethod

   method Action f2h_send (Vector #(F2H_BUF_SIZE_B, Bit #(8)) v,
			   Bit #(16) n_bytes);
      f_send.enq (tuple2 (v, n_bytes));
   endmethod

   method ActionValue #(Vector #(H2F_BUF_SIZE_B, Bit #(8)))
          h2f_recv (Bit #(16) n_bytes);
      actionvalue
	 let v <- pop (f_recv);
	 return v;
      endactionvalue
   endmethod

   method Action finish () if (rg_run_state == STATE_HDR);
      rg_run_state <= STATE_DISCONNECT;
   endmethod

endmodule

// ****************************************************************

endpackage

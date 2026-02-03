// Copyright (c) 2026 Rishiyur S. Nikhil. All Rights Reserved.

package VF_HW_L2;

// ****************************************************************
// Implements FPGA-side of the multi-queue structure.

// WARNING! This is a template file to processed by a Python program
// Gen_VF_HW_L2.py which will substitute certain placeholders with
// App-specific info (# of queues, type of each queue, depth of each
// queue, etc)

// ****************************************************************
// Imports from BSV lib

import FIFOF       :: *;
import Connectable :: *;
import Vector      :: *;

// ----------------
// Imports from additional libs

import Semi_FIFOF :: *;
import GetPut_Aux :: *;

// ----------------
// Imports from this project

import VF_HW_L2_IFC :: *;
import VF_HW_L1     :: *;

// ****************************************************************

Integer l2_verbosity = 1;

// ****************************************************************
// Queue identifiers: 0..254 for each direction.
// 

typedef Bit #(8) Qid;
// Actual queues have 0 <= qid < 8'FE (254)

// Reserved qids
Qid qid_NOP  = 8'h_FF;
Qid qid_CRED = 8'h_FE;

// ****************************************************************
// For debugging

function Action show_Queue_State (Integer qid,
				  String pre,
				  Bit #(16) size_I,
				  Bit #(16) credits_I,
				  String post);
   action
      $write ("%s", pre);
      $write ("Queue[%d] size_I %0d  cred_I %0d", qid, size_I, credits_I);
      $write ("%s", post);
   endaction
endfunction

// ================================================================
// IMPLEMENTATION MODULE

typedef enum {L2_STATE_INITIAL,
	      L2_STATE_STARTING,
	      L2_STATE_RUNNING } L2_State
deriving (Bits, Eq, FShow);

(* synthesize *)
module mkVF_HW_L2 (VF_HW_L2_IFC);

   Reg #(L2_State) rg_state <- mkReg (L2_STATE_INITIAL);

   // ----------------
   // Queue instances

   // GEN_INSERT queue_FIFOs

   // ----------------
   // L1 layer
   VF_HW_L1_IFC layer1 <- mkVF_HW_L1;

   // ================================================================
   // BEHAVIOR

   rule rl_starting (rg_state == L2_STATE_STARTING);
      if (l2_verbosity != 0)
	 $display ("  VF_HW_L2: rl_starting");

      // GEN_INSERT queue_inits

      layer1.start ();

      rg_state <= L2_STATE_RUNNING;
   endrule

   // ----------------
   function Action fa_credit_update (Bit #(8)  qid,
				     Reg #(Bit #(16)) rg_credits_I,
				     Bit #(16) cred_I);
      action
	 let new_cred_I = rg_credits_I + cred_I;
	 rg_credits_I <= new_cred_I;
	 if (l2_verbosity != 0) begin
	    $write   ("    L2.rl_recv_h2f_hdr: CRED");
	    $display (" for remote q[%0d] cred %0d + %0d -> %0d",
		      qid, rg_credits_I, cred_I, new_cred_I);
	 end
      endaction
   endfunction

   // ----------------
   // H2F queues

   Reg #(Qid)       rg_h2f_qid     <- mkReg (qid_NOP);
   Reg #(Bit #(16)) rg_h2f_n_I     <- mkReg (0);
   Reg #(Bit #(8))  rg_h2f_width_B <- mkReg (0);

   Bit #(8) h2f_state_IDLE = 0;
   Bit #(8) h2f_state_CRED = 1;
   Bit #(8) h2f_state_DATA = 2;

   // Receive HDR bytes (could be NOP, CRED, qid)
   rule rl_recv_h2f_hdr ((rg_state == L2_STATE_RUNNING)
			 && (rg_h2f_qid == qid_NOP));
      Vector #(H2F_BUF_SIZE_B, Bit #(8)) v = replicate (0);
      v <- layer1.h2f_recv (4);

      if (v[0] == qid_NOP) begin
	 if (l2_verbosity != 0)
	    $display ("    L2.rl_recv_h2f_hdr: NOP ----------------");
      end
      else if (v [0] == qid_CRED) begin
	 // Credit-report for an F2H queuee
	 let qid    = v[1];
	 let cred_I = { v[3], v[2] };
	 case (qid)
	    // GEN_INSERT credit_update
	    default:
	    begin
	       $display ("ERROR: rl_recv_h2f_hdr: credit-upd for f2h q[%0d]",
			 qid);
	       $display ("    No such queue; ignoring");
	    end
	 endcase
      end
      else begin
	 // Data
	 let qid = v[0];
	 rg_h2f_qid <= qid;
	 Bit #(16) n_I = { v[2], v[1] };
	 rg_h2f_n_I     <= n_I;
	 rg_h2f_width_B <= v[3];
	 if (l2_verbosity != 0)
	    $display ("    L2.rl_recv_h2f_hdr: DATA for qid %0d n_I %0d",
		      qid, n_I);
      end
   endrule

   // GEN_INSERT rl_recv_h2f_qJ_data_item

   // ----------------
   // F2H: send queue data

   Reg #(Qid)       rg_f2h_qid <- mkReg (qid_NOP);
   Reg #(Bit #(16)) rg_f2h_n_I <- mkReg (0);

   // GEN_INSERT rl_send_f2h_qJ

   // ----------------
   // F2H: send credit update for H2F queue

   // GEN_INSERT rl_f2h_credit_for_h2f_qJ

   // ----------------
   // Prioritize rules

   // GEN_INSERT rule_priorities

   // ================================================================
   // INTERFACE

   method Action start () if (rg_state == L2_STATE_INITIAL);
      rg_state <= L2_STATE_STARTING;
   endmethod

   // GEN_INSERT queue_ifc_defs

   method Action finish ();
      layer1.finish ();
   endmethod
endmodule

// ****************************************************************

endpackage

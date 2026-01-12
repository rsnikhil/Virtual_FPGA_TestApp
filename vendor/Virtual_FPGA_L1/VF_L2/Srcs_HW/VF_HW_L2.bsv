// Copyright (c) 2026 Rishiyur S. Nikhil. All Rights Reserved.

package VF_HW_L2;

// ****************************************************************
// Implements FPGA-side of the multi-queue structure.
// This is hand-written, but should ultimately be tool-generated,
// given just the queue size params.

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

import VF_HW_L1 :: *;

// ****************************************************************

Integer l2_verbosity = 1;

// ****************************************************************
// Terminology/notation:

// 'H2F'           Host-to-FPGA
// 'F2H'           FPGA-to-Host
// '..._b'         size measured in bits
// '..._B'         size measured in Bytes (Bit #(8))
// '..._I'         size measured in queue items

// Each 'queue' contains 'items' (which can be different for different queues)
// Each 'item'  is some uninterpreted Bit #(b) data.
//              'b' should be a multiple of 8 (i.e., some # of bytes)
//              to simplify host-FPGA communications.
//              Hence, we specify 'width_B' and derive 'width_b'.

// ****************************************************************
// App-specified queue parameters:
//   width    of a queue-item in bytes
//   capacity of queue in items
// Note: width is in bytes to simplify host-FPGA communication
// App should zeroExtend from, or truncate to, desired width in bits.

// ----------------
// Queues from host to hardware

typedef   8  H2F_q0_width_B;
typedef 128  H2F_q0_capacity_I;

// ----------------
// Queues from hardware to host

typedef   4  F2H_q0_width_B;
typedef  32  F2H_q0_capacity_I;

// ================================================================
// Derived types and values from queue params

// ----------------
// Queues from host to hardware
typedef TMul #(8, H2F_q0_width_B)  H2F_q0_width_b;
Integer h2f_q0_width_b    = valueOf (H2F_q0_width_b);
Integer h2f_q0_width_B    = valueOf (H2F_q0_width_B);
Integer h2f_q0_capacity_I = valueOf (H2F_q0_capacity_I);

// ----------------
// Queues from hardware to host
typedef TMul #(8, F2H_q0_width_B)  F2H_q0_width_b;
Integer f2h_q0_width_b    = valueOf (F2H_q0_width_b);
Integer f2h_q0_width_B    = valueOf (F2H_q0_width_B);
Integer f2h_q0_capacity_I = valueOf (F2H_q0_capacity_I);

// ... and similarly for F2H q1, q2, ...

// ================================================================
// Queue identifiers: 0..254 for each direction.
// 

typedef Bit #(8) Qid;
// Actual queues have 0 <= qid < 8'FE (254)

// Reserved qids
Qid qid_NOP  = 8'h_FF;
Qid qid_CRED = 8'h_FE;

// ****************************************************************
// INTERFACE to App (above this layer)

interface VF_HW_L2_IFC;
   // ----------------
   method Action start ();

   // ----------------
   // ... TODO: Clocks

   // ----------------
   // H2F queues
   interface FIFOF_O #(Bit #(H2F_q0_width_b)) fo_h2f_q0;
   // ... and similarly for h2f_{q1,q2,...}

   // ----------------
   // F2H queues
   interface FIFOF_I #(Bit #(F2H_q0_width_b)) fi_f2h_q0;
   // ... and similarly for f2h_{q1,q2,...}

   // ----------------
   // ... TODO: AXI4 ifcs to DDR(s) 

   // ----------------
   // ... TODO: AXI4 ifcs to other: GPIO, UART, Flash/ROM, Ethernet, USB, ...

   // ----------------
   method Action finish ();
endinterface

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

// Sanity check that width_B in message matches queue's width_B
function Action check_width (Qid qid, Bit #(8) width_B);
   action
      Bool ok;
      if (qid == 0)
	 ok = (width_B == fromInteger (h2f_q0_width_B));
      else
	 ok = False;

      if (! ok) begin
	 $display ("INTERNAL ERROR: in msg for queue %0d: incorrect width %0d",
		   qid, width_B);
	 $finish (1);
      end
   endaction
endfunction

// ================================================================
// IMPLEMENTATION MODULE

typedef enum { L2_STATE_STARTING, L2_STATE_RUNNING } L2_State
deriving (Bits, Eq, FShow);

(* synthesize *)
module mkVF_HW_L2 (VF_HW_L2_IFC);

   Reg #(L2_State) rg_state <- mkReg (L2_STATE_STARTING);

   // ----------------
   // H2F queues
   FIFOF #(Bit #(H2F_q0_width_b)) f_h2f_q0 <- mkSizedFIFOF (h2f_q0_capacity_I);
   Reg #(Bit #(16)) rg_h2f_q0_size_I    <- mkRegU;
   Reg #(Bit #(16)) rg_h2f_q0_credits_I <- mkRegU;

   // ----------------
   // F2H queues
   FIFOF #(Bit #(F2H_q0_width_b)) f_f2h_q0 <- mkSizedFIFOF (f2h_q0_capacity_I);
   Reg #(Bit #(16)) rg_f2h_q0_size_I    <- mkReg (unpack (0));
   Reg #(Bit #(16)) rg_f2h_q0_credits_I <- mkReg (unpack (0));

   // L1 layer
   VF_HW_L1_IFC layer1 <- mkVF_HW_L1;

   // ================================================================
   // BEHAVIOR

   // ----------------
   // H2F queues

   Reg #(Qid)       rg_h2f_qid <- mkReg (qid_NOP);
   Reg #(Bit #(16)) rg_h2f_n_I <- mkReg (0);

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
	    0: begin
		  let new_cred_I = rg_f2h_q0_credits_I + cred_I;
		  rg_f2h_q0_credits_I <= new_cred_I;
		  if (l2_verbosity != 0) begin
		     $write   ("    L2.rl_recv_h2f_hdr: CRED");
		     $display (" for remote q[0] cred %0d + %0d -> %0d",
			       rg_f2h_q0_credits_I, cred_I, new_cred_I);
		  end
	       end
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
	 rg_h2f_n_I <= n_I;
	 check_width (qid, v[3]);
	 if (l2_verbosity != 0)
	    $display ("    L2.rl_recv_h2f_hdr: DATA for qid %0d n_I %0d",
		      qid, n_I);
      end
   endrule

   // For h2f msg; rec'd hdr; now rec'v n_I data items
   rule rl_recv_h2f_q0_data_item ((rg_state == L2_STATE_RUNNING)
				  && (rg_h2f_qid == 0)
				  && (rg_h2f_n_I != 0));
      Vector #(H2F_BUF_SIZE_B, Bit #(8))
         v <- layer1.h2f_recv (fromInteger (h2f_q0_width_B));

      Bool data_recd = (last (v) == 1);
      if (data_recd) begin
	 Bit #(H2F_q0_width_b) item = truncate (pack (v));
	 if (l2_verbosity != 0)
	    $display ("    item %0h n_I = %0d (L2.rl_recv_h2f_q0_data_item)",
		      item, rg_h2f_n_I);
	 f_h2f_q0.enq (item);
	 rg_h2f_q0_size_I <= rg_h2f_q0_size_I + 1;
	 let new_n_I = rg_h2f_n_I - 1;
	 if (new_n_I == 0)
	    rg_h2f_qid <= qid_NOP;
	 rg_h2f_n_I <= new_n_I;
      end
   endrule

   // ----------------
   // F2H: send queue data

   Reg #(Qid)       rg_f2h_qid <- mkReg (qid_NOP);
   Reg #(Bit #(16)) rg_f2h_n_I <- mkReg (0);

   rule rl_send_f2h_q0_hdr ((rg_state == L2_STATE_RUNNING)
			    && (rg_f2h_qid == qid_NOP)
			    && (rg_f2h_q0_size_I != 0)
			    && (rg_f2h_q0_credits_I != 0));
      Bit #(16) n_I = min (rg_f2h_q0_size_I, rg_f2h_q0_credits_I);
      Vector #(F2H_BUF_SIZE_B, Bit #(8)) v = ?;
      v [0] = 0;
      v [1] = n_I [7:0];
      v [2] = n_I [15:8];
      v [3] = fromInteger (f2h_q0_width_B);
      layer1.f2h_send (v, 4);
      rg_f2h_n_I <= n_I;
      rg_f2h_qid <= 0;
      if (l2_verbosity != 0)
	 $display ("    L2.rl_send_f2h_q0_hdr: qid 0 n_I %0d", n_I);
   endrule

   // This rule iterates n_I times, sending one item each time
   rule rl_send_f2h_q0_data_items ((rg_state == L2_STATE_RUNNING)
				   && (rg_f2h_qid == 0)
				   && rg_f2h_n_I != 0);
      let item <- pop (f_f2h_q0);
      if (l2_verbosity != 0)
	 $display ("    item %0h  n_I %0d (L2.rl_send_f2h_q0_data_item)",
		   item, rg_f2h_n_I);
      rg_f2h_q0_size_I    <= rg_f2h_q0_size_I    - 1;
      rg_f2h_q0_credits_I <= rg_f2h_q0_credits_I - 1;
      Vector #(F2H_BUF_SIZE_B, Bit #(8)) v = unpack (zeroExtend (item));
      layer1.f2h_send (v, fromInteger (f2h_q0_width_B));
      if (rg_f2h_n_I == 1)
	 rg_f2h_qid <= qid_NOP;
      rg_f2h_n_I <= rg_f2h_n_I - 1;
   endrule

   // ----------------
   // F2H: send credit update for H2F queue

   // Prioritize credit-update over data-send
   (* preempts = "rl_f2h_credit_for_h2f_q0, rl_send_f2h_q0_hdr" *)
   (* preempts = "rl_f2h_credit_for_h2f_q0, rl_send_f2h_q0_data_items" *)
   rule rl_f2h_credit_for_h2f_q0 ((rg_state == L2_STATE_RUNNING)
				  && (rg_f2h_qid == qid_NOP)
				  && (rg_h2f_q0_credits_I != 0));
      Vector #(F2H_BUF_SIZE_B, Bit #(8)) v = ?;
      v [0] = qid_CRED;
      v [1] = 0;
      v [2] = rg_h2f_q0_credits_I [7:0];
      v [3] = rg_h2f_q0_credits_I [15:8];
      layer1.f2h_send (v, 4);
      if (l2_verbosity != 0)
	 $display ("    L2.rl_f2h_credit_for_h2f_q0: %0d",
		   rg_h2f_q0_credits_I);

      rg_h2f_q0_credits_I <= 0;
   endrule

   // ================================================================
   // INTERFACE

   method Action start () if (rg_state == L2_STATE_STARTING);
      $display ("VF_HW_L2: start()");

      rg_h2f_q0_size_I    <= 0;
      rg_h2f_q0_credits_I <= fromInteger (h2f_q0_capacity_I);

      rg_f2h_q0_size_I    <= 0;
      rg_f2h_q0_credits_I <= 0;

      layer1.start ();
      rg_state <= L2_STATE_RUNNING;
   endmethod

   interface FIFOF_O fo_h2f_q0;
      method notEmpty = f_h2f_q0.notEmpty;
      method first    = f_h2f_q0.first;
      method deq;
	 action
	    f_h2f_q0.deq;
	    rg_h2f_q0_credits_I <= rg_h2f_q0_credits_I + 1;
	 endaction
      endmethod
   endinterface

   interface FIFOF_I fi_f2h_q0;
      method notFull = f_f2h_q0.notFull;
      method Action enq (Bit #(F2H_q0_width_b) x);
	 action
	    f_f2h_q0.enq (x);
	    rg_f2h_q0_size_I <= rg_f2h_q0_size_I + 1;
	 endaction
      endmethod
   endinterface

   method Action finish ();
      layer1.finish ();
   endmethod
endmodule

// ****************************************************************

endpackage

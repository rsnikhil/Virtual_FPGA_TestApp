// Copyright (c) 2026 Rishiyur S. Nikhil

package App_HW;

// ================================================================
// This package contains the mkApp_HW module
// which is instantiated in the Virtual FPGA environment
// and runs as the Virtual FPGA HW-side.

// ================================================================
// BSV library imports

import Clocks      :: *;
import FIFOF       :: *;
import Connectable :: *;

// ----------------
// Imports from additional libs

import AXI4_BSV_RTL :: *;
import Semi_FIFOF   :: *;

// ----------------
// Imports from this project

import VF_HW_L2 :: *;

// ****************************************************************
// AXI4 Params for this DUT

typedef 16  DDR_AXI4_wd_id;
typedef 64  DDR_AXI4_wd_addr;
typedef 512 DDR_AXI4_wd_data;
typedef 0   DDR_AXI4_wd_user;

// ****************************************************************
// DUT Interface

interface App_HW_IFC;
   method ActionValue #(Bool) start ();
   method Bool   done;
   method Action finish ();
endinterface

// ****************************************************************
// DUT Module

module mkApp_HW
   #(Clock clk1, Clock clk2, Clock clk3, Clock clk4, Clock clk5,
     FIFOF_O #(Bit #(H2F_q0_width_b)) fo_h2f,
     FIFOF_I #(Bit #(F2H_q0_width_b)) fi_f2h

     // DDR
     // AXI4_RTL_S_IFC #(DDR_AXI4_wd_id,
     //		         DDR_AXI4_wd_addr,
     //                  DDR_AXI4_wd_data,
     //                  DDR_AXI4_wd_user) ddr_AXI4_RTL_S
     )
   (App_HW_IFC);

   /*
   // DDR AXI4 Transactors
   AXI4_BSV_to_RTL_IFC #(DDR_AXI4_wd_id,
			 DDR_AXI4_wd_addr,
			 DDR_AXI4_wd_data,
			 DDR_AXI4_wd_user) ddr_xactor <- mkAXI4_BSV_to_RTL;

   mkConnection (ddr_xactor.rtl_M, ddr_AXI4_RTL_S);
   */

   // ================================================================
   // BEHAVIOR
   // Simple FSM to echo data from host back to host

   Reg #(Bit #(2))              rg_state <- mkReg (0);
   Reg #(Bit #(H2F_q0_width_b)) rg_data  <- mkRegU;

   rule rl_echo_0 (rg_state == 0);
      let x <- pop_o (fo_h2f);
      $display ("App: rl_echo_0: recv %0h", x);
      rg_data  <= x;
      rg_state <= 1;
   endrule

   rule rl_echo_1 (rg_state == 1);
      let x = rg_data [31:0];
      fi_f2h.enq (x);
      $display ("App: rl_echo_1: send %0h", x);
      rg_state <= 2;
   endrule

   rule rl_echo_2 (rg_state == 2);
      let x = rg_data [63:32];
      fi_f2h.enq (x);
      $display ("App: rl_echo_2: send %0h", x);
      rg_state <= 0;
   endrule

   // ================================================================
   // INTERFACE

   method ActionValue #(Bool) start ();
      return True;
   endmethod

   method Bool done;
      return False;
   endmethod

   method Action finish ();
      noAction;
   endmethod
endmodule

// ================================================================

endpackage

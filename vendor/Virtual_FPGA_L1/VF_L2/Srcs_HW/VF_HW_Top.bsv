// Copyright (c) 2026 Rishiyur S. Nikhil. All Rights Reserved.

package VF_HW_Top;

// ================================================================
// This implements the Virtual SW-HW system top-level.
// 
// It instantiates VF_HW_L2 (layer 2) (which includes layer 1).

// It instantiates App_HW, passing it layer 2's interfaces for host
// communications, DDR access, and other resource access.

// ================================================================
// Imports from BSV lib

import Clocks      :: *;
import FIFOF       :: *;
import Connectable :: *;
import Vector      :: *;

// ----------------
// Imports from additional libs

import Semi_FIFOF   :: *;

// ----------------
// Imports from this project

import VF_HW_L2 :: *;    // Layer 2 of infra (includes Layer 1)

// The application, hardware side
import App_HW :: *;

// ****************************************************************
// MODULE

(* synthesize *)
module mkVF_HW_Top (Empty);

   // Instantiate Layer 2 (which includes Layer 1)
   VF_HW_L2_IFC layer2 <- mkVF_HW_L2;


   ClockDividerIfc clkdiv1 <- mkClockDivider (2);    // 125 MHz
   ClockDividerIfc clkdiv2 <- mkClockDivider (3);    // 83.333 MHz
   ClockDividerIfc clkdiv3 <- mkClockDivider (5);    // 50 MHz
   ClockDividerIfc clkdiv4 <- mkClockDivider (10);   // 25 MHz
   ClockDividerIfc clkdiv5 <- mkClockDivider (25);   // 10 MHz

   // The HW-side of the Application
   App_HW_IFC app <- mkApp_HW (clkdiv1.slowClock,
			       clkdiv2.slowClock,
			       clkdiv3.slowClock,
			       clkdiv4.slowClock,
			       clkdiv5.slowClock,
			       layer2.fo_h2f_q0,
			       layer2.fi_f2h_q0
			       // AXI4(s) to DDR(s)
			       // AXI4(s) to other resource(s)
			       );

   // ================================================================
   // BEHAVIOR

   Reg #(Bit #(8)) rg_step <- mkReg (0);

   rule rl_step0 (rg_step == 0);
      $display ("========================================================");
      $display ("VF_HW_Top (simulation)");
      $display ("Copyright (c) 2026 R.S.Nikhil. All Rights Reserved.");
      $display ("========================================================");
      rg_step <= rg_step + 1;
   endrule

   rule rl_step1 (rg_step == 1);
      $display ("VF_HW_Top: layer2.start()");
      layer2.start ();
      rg_step <= rg_step + 1;
   endrule

   rule rl_step2 (rg_step == 2);
      Bool ok <- app.start ();
      if (! ok) begin
	 $display ("ERROR: VF_HW_Top: app.start() failed");
	 $finish (1);
      end
      else
	 rg_step <= rg_step + 1;
   endrule

   rule rl_step3 ((rg_step == 3) && app.done);
      $display ("========================================================");
      $display ("VF_HW_Top (simulation) ... Received app.done()");
      $display ("    ... finishing");
      $display ("========================================================");
      app.finish ();
      layer2.finish ();
      rg_step <= rg_step + 1;
   endrule

   rule rl_step4 (rg_step == 4);
      $finish (0);
   endrule

   // ================================================================
   // INTERFACE

   //  Empty (this is top-level)

endmodule

endpackage

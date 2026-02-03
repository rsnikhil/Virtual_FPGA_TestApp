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

import VF_HW_L2_IFC :: *;    // Interface of infra layer 2 (includes Layer 1)
import VF_HW_L2     :: *;    // Infra layer 2 (includes Layer 1)

// The application, hardware side
import App_HW :: *;

// ****************************************************************
// MODULE

(* synthesize *)
module mkVF_HW_Top (Empty);

   // Instantiate Layer 2 (which includes Layer 1)
   VF_HW_L2_IFC layer2 <- mkVF_HW_L2;

   // The HW-side of the Application
   App_HW_IFC app <- mkApp_HW (layer2);

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
      $display ("VF_HW_Top: app.start()");
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

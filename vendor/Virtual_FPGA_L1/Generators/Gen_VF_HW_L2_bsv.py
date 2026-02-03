#!/usr/bin/python3

# ****************************************************************
# Copyright (c) 2026 Rishiyur S. Nikhil. All Rights Reserved

# ****************************************************************

def print_usage (argv):
    sys.stdout.write ("Usage:\n")
    sys.stdout.write ("  {0}  <qspecs>  <template>\n"
                      .format (argv [0]))
    sys.stdout.write ("  where:\n")
    sys.stdout.write ("    <qspecs>:   text file describing L2 queues\n")
    sys.stdout.write ("    <template>: e.g., VF_HW_L2_TEMPLATE.bsv\n")
    sys.stdout.write ("Generates:\n")
    sys.stdout.write ("  VF_HW_L1_Params.bsv (imported by VF_HW_L1.bsv)\n")
    sys.stdout.write ("  VF_HW_L2.bsv        (HW-side multi-queues)\n")
    sys.stdout.write ("  VF_HW_L2_IFC.bsv    (param to user's BSV App)\n")
 
# ****************************************************************

debug = True

# ****************************************************************
# Import standard libs

import sys

# Import local

from Read_L2_MultiQueue_Spec import *

# ****************************************************************

def main (argv = None):
    if (len (argv) != 3) or ("-h" in argv) or ("--help" in argv):
        print_usage (argv)
        return 0

    qspecs_filename   = argv [1]
    template_filename = argv [2]

    params_filename  = "VF_HW_L1_Params.bsv"
    L2_name          = "VF_HW_L2"
    L2_filename      = L2_name + ".bsv"
    L2_IFC_name      = L2_name + "_IFC"
    L2_IFC_filename  = L2_IFC_name + ".bsv"

    (qspecs_h2f, qspecs_f2h) = read_L2_MultiQueue_Spec (qspecs_filename)
    if debug:
        print_L2_MultiQueue_Specs (qspecs_h2f, qspecs_f2h)

    # ----------------
    # Generate VF_HW_L1_Params.bsv
    sys.stdout.write ('Generating "{:s}" ...\n'.format (params_filename))
    h2f_max_item_width_B = max ([ qs ["width_B"] for qs in qspecs_h2f ])
    f2h_max_item_width_B = max ([ qs ["width_B"] for qs in qspecs_f2h ])
    with open (params_filename, "w") as fpo:
        fpo.write ("// THIS FILE IS GENERATED; DO NOT EDIT.\n")
        fpo.write ("\n")
        fpo.write ("package VF_HW_L1_Params;\n")
        fpo.write ("\n")
        fpo.write ("// Max queue-width params for L1 layer\n")
        fpo.write ("\n")
        fpo.write ("typedef {:2d} H2F_MAX_ITEM_WIDTH_B;\n"
                   .format (h2f_max_item_width_B))
        fpo.write ("typedef {:2d} F2H_MAX_ITEM_WIDTH_B;\n"
                   .format (f2h_max_item_width_B))
        fpo.write ("\n")
        fpo.write ("endpackage\n")
    sys.stdout.write ("  ... done\n")

    # ----------------
    # Generate VF_HW_L2_IFC.bsv

    sys.stdout.write ('Generating "{:s}" ...\n'.format (L2_IFC_filename))
    with open (L2_IFC_filename, "w") as fpo:
        fpo.write ("// THIS FILE IS GENERATED; DO NOT EDIT\n")
        fpo.write ("\n")
        fpo.write ("package {:s};\n".format (L2_IFC_name))
        fpo.write ("\n")
        emit_queue_params (fpo, qspecs_h2f, qspecs_f2h)
        fpo.write ("\n")
        emit_L2_IFC_decl (fpo, qspecs_h2f, qspecs_f2h)
        fpo.write ("\n")
        fpo.write ("endpackage\n")
    sys.stdout.write ("  ... done\n")

    # ----------------
    # Generate VF_HW_L2.bsv

    sys.stdout.write ('Generating "{:s}" ...\n'.format (L2_filename))
    with open (L2_filename, "w") as fpo:
        fpo.write ("// THIS FILE IS GENERATED; DO NOT EDIT\n")
        fpo.write ("\n")
        for line in open (template_filename, "r"):
            fpo.write (line)
            if "GEN_INSERT queue_FIFOs" in line:
                emit_queue_FIFOs (fpo, qspecs_h2f, qspecs_f2h)
            elif "GEN_INSERT credit_update" in line:
                emit_queue_credit_update (fpo, qspecs_h2f, qspecs_f2h)
            elif "GEN_INSERT rl_recv_h2f_qJ_data_item" in line:
                emit_rl_recv_h2f_qJ_data_item_template (fpo,
                                                        qspecs_h2f,
                                                        qspecs_f2h)
            elif "GEN_INSERT rl_send_f2h_qJ" in line:
                emit_rl_send_f2h_qJ (fpo, qspecs_h2f, qspecs_f2h)
            elif "GEN_INSERT rl_f2h_credit_for_h2f_qJ" in line:
                emit_rl_f2h_credit_for_h2f_qJ (fpo, qspecs_h2f, qspecs_f2h)
            elif "GEN_INSERT queue_inits" in line:
                emit_queue_inits (fpo, qspecs_h2f, qspecs_f2h)
            elif "GEN_INSERT queue_ifc_defs" in line:
                emit_queue_ifc_defs (fpo, qspecs_h2f, qspecs_f2h)
            elif "GEN_INSERT rule_priorities" in line:
                emit_rule_priorities (fpo, qspecs_h2f, qspecs_f2h)
    sys.stdout.write ("  ... done\n")

    return 0

# ****************************************************************

queue_params_boilerplate = [
    "// ****************************************************************",
    "// Imports from BSV lib",
    "",
    "import FIFOF       :: *;",
    "",
    "// ----------------",
    "// Imports from additional libs",
    "",
    "import Semi_FIFOF :: *;",
    "",
    "// ****************************************************************",
    "// Terminology/notation:",
    "",
    "// 'H2F'           Host-to-FPGA",
    "// 'F2H'           FPGA-to-Host",
    "// '..._b'         size measured in bits",
    "// '..._B'         size measured in Bytes (Bit #(8))",
    "// '..._I'         size measured in queue items",
    "",
    "// Each 'queue' has 'items' (can be different for different queues),",
    "// Each 'item'  is some uninterpreted Bit #(b) data.",
    "//              'b' should be a multiple of 8 (i.e., some # of bytes)",
    "//              to simplify host-FPGA communications.",
    "//              Hence, we specify 'width_B' and derive 'width_b'."
    "",
    "// ****************************************************************",
    "// App-specified queue parameters:",
    "//   width    of a queue-item in bytes",
    "//   capacity of queue in items",
    "// Note: width is in bytes to simplify host-FPGA communication",
    "// App should zeroExtend from, or truncate to, desired width in bits.",
    ""
]

def emit_queue_params (fpo, qspecs_h2f, qspecs_f2h):
    for line in queue_params_boilerplate:
        fpo.write ("{:s}\n".format (line))

    fpo.write ("// ----------------\n")
    fpo.write ("// H2F queue params\n")
    for j in range (len (qspecs_h2f)):
        qdj = qspecs_h2f [j]
        qJ  = "q{:0d}".format (j)
        fpo.write ("\n")
        fpo.write ("typedef {:4d} H2F_{:s}_width_B;\n"
                   .format (qdj ["width_B"], qJ))
        fpo.write ("typedef {:4d} H2F_{:s}_capacity_I;\n"
                   .format (qdj ["capacity_f_I"], qJ))
        fpo.write ("typedef TMul #(8, H2F_{:s}_width_B)  H2F_{:s}_width_b;\n"
                   .format (qJ, qJ))
        fpo.write ("Integer h2f_{:s}_width_b    = valueOf (H2F_{:s}_width_b);\n"
                   .format (qJ, qJ))
        fpo.write ("Integer h2f_{:s}_width_B    = valueOf (H2F_{:s}_width_B);\n"
                   .format (qJ, qJ))
        fpo.write ("Integer h2f_{:s}_capacity_I = valueOf (H2F_{:s}_capacity_I);\n"
                   .format (qJ, qJ))

    fpo.write ("\n")
    fpo.write ("// ----------------\n")
    fpo.write ("// F2H queue params\n")
    for j in range (len (qspecs_f2h)):
        qdj = qspecs_f2h [j]
        qJ  = "q{:0d}".format (j)
        fpo.write ("\n")
        fpo.write ("typedef {:4d} F2H_{:s}_width_B;\n"
                   .format (qdj ["width_B"], qJ))
        fpo.write ("typedef {:4d} F2H_{:s}_capacity_I;\n"
                   .format (qdj ["capacity_h_I"], qJ))
        fpo.write ("typedef TMul #(8, F2H_{:s}_width_B)  F2H_{:s}_width_b;\n"
                   .format (qJ, qJ))
        fpo.write ("Integer f2h_{:s}_width_b    = valueOf (F2H_{:s}_width_b);\n"
                   .format (qJ, qJ))
        fpo.write ("Integer f2h_{:s}_width_B    = valueOf (F2H_{:s}_width_B);\n"
                   .format (qJ, qJ))
        fpo.write ("Integer f2h_{:s}_capacity_I = valueOf (F2H_{:s}_capacity_I);\n"
                   .format (qJ, qJ))

# ****************************************************************

ifc_decl_boilerplate_1 = [
    "// ****************************************************************",
    "// INTERFACE to App (above this layer)",
    "",
    "interface VF_HW_L2_IFC;",
    "   // ----------------",
    "   method Action start ();",
    "",
    "   // ----------------",
    "   // ... TODO: Clocks",
    "",
    "   // ----------------",
    "   // Multi-queue interface declarations",
    ""
]

ifc_decl_template_1 = (
    "   interface FIFOF_O #(Bit #(H2F_qJ_width_b)) fo_h2f_qJ;\n")

ifc_decl_template_2 = (
    "   interface FIFOF_I #(Bit #(F2H_qJ_width_b)) fi_f2h_qJ;\n")

ifc_decl_boilerplate_2 = [
    "",
    "   // ----------------",
    "   // ... TODO: AXI4 ifcs to DDR(s) ",
    "",
    "   // ----------------",
    "   // ... TODO: AXI4 ifcs to other resources:",
    "   //     e.g., GPIO, UART, Flash/ROM, Ethernet, USB, ...",
    "",
    "   // ----------------",
    "   method Action finish ();",
    "endinterface"
]

def emit_L2_IFC_decl (fpo, qspecs_h2f, qspecs_f2h):
    for line in ifc_decl_boilerplate_1:
        fpo.write ("{:s}\n".format (line))

    fpo.write ("   // ----------------\n")
    fpo.write ("   // H2F queue ifc decls\n")
    for j in range (len (qspecs_h2f)):
        qJ  = "q{:0d}".format (j)
        fpo.write (ifc_decl_template_1.replace ("qJ", qJ))

    fpo.write ("\n")
    fpo.write ("   // ----------------\n")
    fpo.write ("   // F2H queue ifc decls\n")
    for j in range (len (qspecs_f2h)):
        qJ  = "q{:0d}".format (j)
        fpo.write (ifc_decl_template_2.replace ("qJ", qJ))

    for line in ifc_decl_boilerplate_2:
        fpo.write ("{:s}\n".format (line))

# ****************************************************************

FIFO_template_1 = (
    "\n"
    + "   FIFOF #(Bit #(H2F_qJ_width_b)) f_h2f_qJ <- mkSizedFIFOF (h2f_qJ_capacity_I);\n"
    + "   Reg #(Bit #(16)) rg_h2f_qJ_size_I    <- mkRegU;\n"
    + "   Reg #(Bit #(16)) rg_h2f_qJ_credits_I <- mkRegU;\n"
)

FIFO_template_2 = (
    "\n"
    + "   FIFOF #(Bit #(F2H_qJ_width_b)) f_f2h_qJ <- mkSizedFIFOF (f2h_qJ_capacity_I);\n"
    + "   Reg #(Bit #(16)) rg_f2h_qJ_size_I    <- mkRegU;\n"
    + "   Reg #(Bit #(16)) rg_f2h_qJ_credits_I <- mkRegU;\n"
)

def emit_queue_FIFOs (fpo, qspecs_h2f, qspecs_f2h):
    fpo.write ("   // ----------------\n")
    fpo.write ("   // H2F queue FIFOs\n")
    for j in range (len (qspecs_h2f)):
        qJ  = "q{:0d}".format (j)
        fpo.write (FIFO_template_1.replace ("qJ", qJ))

    fpo.write ("   // ----------------\n")
    fpo.write ("   // F2H queue FIFOs\n")
    for j in range (len (qspecs_f2h)):
        qJ  = "q{:0d}".format (j)
        fpo.write (FIFO_template_2.replace ("qJ", qJ))

# ****************************************************************

credit_update_template = (
    "	    J: fa_credit_update (J, rg_f2h_qJ_credits_I, cred_I);\n"
)

def emit_queue_credit_update (fpo, qspecs_h2f, qspecs_f2h):
    for j in range (len (qspecs_f2h)):
        qJ  = "q{:0d}".format (j)
        s1 = credit_update_template.replace ("qJ", qJ)
        s2 = s1.replace ("J", str (j))
        fpo.write (s2)


# ****************************************************************

rl_recv_h2f_qJ_data_item_template = (
    "\n"
    + "   // h2f qJ data msg; rec'd hdr; now rec'v n_I data items\n"
    + "   rule rl_recv_h2f_qJ_data_item ((rg_state == L2_STATE_RUNNING)\n"
    + "                                  && (rg_h2f_qid == J)\n"
    + "                                  && (rg_h2f_n_I != 0));\n"
    + "\n"
    + "      if (rg_h2f_width_B != fromInteger (h2f_qJ_width_B)) begin\n"
    + '         $display ("INTERNAL ERROR: in msg for qJ: bad width %0d",\n'
    + "                   rg_h2f_width_B);\n"
    + "         $finish (1);\n"
    + "      end\n"
    + "\n"
    + "      Vector #(H2F_BUF_SIZE_B, Bit #(8))\n"
    + "         v <- layer1.h2f_recv (fromInteger (h2f_qJ_width_B));\n"
    + "\n"
    + "      Bool data_recd = (last (v) == 1);\n"
    + "      if (data_recd) begin\n"
    + "         Bit #(H2F_qJ_width_b) item = truncate (pack (v));\n"
    + "         if (l2_verbosity != 0)\n"
    + '            $display ("    item %0h n_I = %0d (L2.rl_recv_h2f_qJ_data_item)",\n'
    + "                      item, rg_h2f_n_I);\n"
    + "         f_h2f_qJ.enq (item);\n"
    + "         rg_h2f_qJ_size_I <= rg_h2f_qJ_size_I + 1;\n"
    + "         let new_n_I = rg_h2f_n_I - 1;\n"
    + "         if (new_n_I == 0)\n"
    + "            rg_h2f_qid <= qid_NOP;\n"
    + "         rg_h2f_n_I <= new_n_I;\n"
    + "      end\n"
    + "   endrule\n"
)

def emit_rl_recv_h2f_qJ_data_item_template (fpo, qspecs_h2f, qspecs_f2h):
    for j in range (len (qspecs_h2f)):
        qJ  = "q{:0d}".format (j)
        s1 = rl_recv_h2f_qJ_data_item_template.replace ("qJ", qJ)
        s2 = s1.replace ("J", str (j))
        fpo.write (s2)

# ****************************************************************

rl_send_f2h_qJ_template = (
   "   rule rl_send_f2h_qJ_hdr ((rg_state == L2_STATE_RUNNING)\n"
   "                            && (rg_f2h_qid == qid_NOP)\n"
   "                            && (rg_f2h_qJ_size_I != 0)\n"
   "                            && (rg_f2h_qJ_credits_I != 0));\n"
   "      Bit #(16) n_I = min (rg_f2h_qJ_size_I, rg_f2h_qJ_credits_I);\n"
   "      Vector #(F2H_BUF_SIZE_B, Bit #(8)) v = ?;\n"
   "      v [0] = 0;\n"
   "      v [1] = n_I [7:0];\n"
   "      v [2] = n_I [15:8];\n"
   "      v [3] = fromInteger (f2h_qJ_width_B);\n"
   "      layer1.f2h_send (v, 4);\n"
   "      rg_f2h_n_I <= n_I;\n"
   "      rg_f2h_qid <= J;\n"
   "      if (l2_verbosity != 0)\n"
   '         $display ("    L2.rl_send_f2h_qJ_hdr: qid J n_I %0d", n_I);\n'
   "   endrule\n"
   "\n"
   "   // This rule iterates n_I times, sending one item each time\n"
   "   rule rl_send_f2h_qJ_data_items ((rg_state == L2_STATE_RUNNING)\n"
   "                                   && (rg_f2h_qid == J)\n"
   "                                   && rg_f2h_n_I != 0);\n"
   "      let item <- pop (f_f2h_qJ);\n"
   "      if (l2_verbosity != 0)\n"
   '         $display ("    item %0h  n_I %0d (L2.rl_send_f2h_qJ_data_items)",\n'
   "                   item, rg_f2h_n_I);\n"
   "      rg_f2h_qJ_size_I    <= rg_f2h_qJ_size_I    - 1;\n"
   "      rg_f2h_qJ_credits_I <= rg_f2h_qJ_credits_I - 1;\n"
   "      Vector #(F2H_BUF_SIZE_B, Bit #(8)) v = unpack (zeroExtend (item));\n"
   "      layer1.f2h_send (v, fromInteger (f2h_qJ_width_B));\n"
   "      if (rg_f2h_n_I == 1)\n"
   "         rg_f2h_qid <= qid_NOP;\n"
   "      rg_f2h_n_I <= rg_f2h_n_I - 1;\n"
   "   endrule\n"
)

def emit_rl_send_f2h_qJ (fpo, qspecs_h2f, qspecs_f2h):
    for j in range (len (qspecs_f2h)):
        qJ  = "q{:0d}".format (j)
        s1 = rl_send_f2h_qJ_template.replace ("qJ", qJ)
        s2 = s1.replace ("J", str (j))
        fpo.write (s2)

# ****************************************************************

rl_f2h_credit_for_h2f_qJ_template = (
   "   rule rl_f2h_credit_for_h2f_qJ ((rg_state == L2_STATE_RUNNING)\n"
   "                                  && (rg_f2h_qid == qid_NOP)\n"
   "                                  && (rg_h2f_qJ_credits_I != 0));\n"
   "      Vector #(F2H_BUF_SIZE_B, Bit #(8)) v = ?;\n"
   "      v [0] = qid_CRED;\n"
   "      v [1] = J;\n"
   "      v [2] = rg_h2f_qJ_credits_I [7:0];\n"
   "      v [3] = rg_h2f_qJ_credits_I [15:8];\n"
   "      layer1.f2h_send (v, 4);\n"
   "      if (l2_verbosity != 0)\n"
   '         $display ("    L2.rl_f2h_credit_for_h2f_qJ: %0d",\n'
   "                   rg_h2f_qJ_credits_I);\n"
   "\n"
   "      rg_h2f_qJ_credits_I <= 0;\n"
   "   endrule\n"
)

def emit_rl_f2h_credit_for_h2f_qJ (fpo, qspecs_h2f, qspecs_f2h):
    for j in range (len (qspecs_h2f)):
        qJ  = "q{:0d}".format (j)
        s1 = rl_f2h_credit_for_h2f_qJ_template.replace ("qJ", qJ)
        s2 = s1.replace ("J", str (j))
        fpo.write (s2)

# ****************************************************************

h2f_queue_init_template = (
   "\n"
   "      rg_h2f_q0_size_I    <= 0;\n"
   "      rg_h2f_q0_credits_I <= fromInteger (h2f_q0_capacity_I);\n"
)

f2h_queue_init_template = (
   "\n"
   "      rg_f2h_q0_size_I    <= 0;\n"
   "      rg_f2h_q0_credits_I <= 0;\n"
)

def emit_queue_inits (fpo, qspecs_h2f, qspecs_f2h):
    for j in range (len (qspecs_h2f)):
        qJ  = "q{:0d}".format (j)
        s1 = h2f_queue_init_template.replace ("qJ", qJ)
        s2 = s1.replace ("J", str (j))
        fpo.write (s2)

    for j in range (len (qspecs_f2h)):
        qJ  = "q{:0d}".format (j)
        s1 = f2h_queue_init_template.replace ("qJ", qJ)
        s2 = s1.replace ("J", str (j))
        fpo.write (s2)

# ****************************************************************

h2f_queue_ifc_def_template = (
   "\n"
   "   interface FIFOF_O fo_h2f_qJ;\n"
   "      method notEmpty = f_h2f_qJ.notEmpty;\n"
   "      method first    = f_h2f_qJ.first;\n"
   "      method deq;\n"
   "         action\n"
   "            f_h2f_qJ.deq;\n"
   "            rg_h2f_qJ_credits_I <= rg_h2f_qJ_credits_I + 1;\n"
   "         endaction\n"
   "      endmethod\n"
   "   endinterface\n"
)

f2h_queue_ifc_def_template = (
   "   interface FIFOF_I fi_f2h_qJ;\n"
   "      method notFull = f_f2h_qJ.notFull;\n"
   "      method Action enq (Bit #(F2H_qJ_width_b) x);\n"
   "         action\n"
   "            f_f2h_qJ.enq (x);\n"
   "            rg_f2h_qJ_size_I <= rg_f2h_qJ_size_I + 1;\n"
   "         endaction\n"
   "      endmethod\n"
   "   endinterface\n"
)

def emit_queue_ifc_defs (fpo, qspecs_h2f, qspecs_f2h):
    for j in range (len (qspecs_h2f)):
        qJ  = "q{:0d}".format (j)
        s1 = h2f_queue_ifc_def_template.replace ("qJ", qJ)
        s2 = s1.replace ("J", str (j))
        fpo.write (s2)

    for j in range (len (qspecs_f2h)):
        qJ  = "q{:0d}".format (j)
        s1 = f2h_queue_ifc_def_template.replace ("qJ", qJ)
        s2 = s1.replace ("J", str (j))
        fpo.write (s2)

# ****************************************************************

def emit_rule_priorities (fpo, qspecs_h2f, qspecs_f2h):
    xs = []
    for j in range (len (qspecs_h2f)):
        xs.append ('rl_f2h_credit_for_h2f_q{:0d}'.format (j))
    for j in range (len (qspecs_f2h)):
        xs.append ('rl_send_f2h_q{:0d}_hdr'.format (j))
        xs.append ('rl_send_f2h_q{:0d}_data_items'.format (j))
    for j in range (len (xs) - 1):
        fpo.write ('   (* descending_urgency = "{:s}, {:s}" *)\n'
                   .format (xs [j], xs [j+1]))
    fpo.write ("   rule rl_dummy (False);\n")
    fpo.write ("   endrule\n")

# ****************************************************************
# ****************************************************************
# ****************************************************************
# For non-interactive invocations, call main() and use its return value
# as the exit code.
if __name__ == '__main__':
  sys.exit (main (sys.argv))

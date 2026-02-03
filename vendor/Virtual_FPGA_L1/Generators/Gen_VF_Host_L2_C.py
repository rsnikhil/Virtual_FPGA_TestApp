#!/usr/bin/python3

# ****************************************************************
# Copyright (c) 2026 Rishiyur S. Nikhil. All Rights Reserved

# ****************************************************************

def print_usage (argv):
    sys.stdout.write ("Usage:\n")
    sys.stdout.write ("  {0}  <qspecs>\n".format (argv [0]))
    sys.stdout.write ("  where:\n")
    sys.stdout.write ("    <qspecs>:      text file describing L2 queues\n")
    sys.stdout.write ("    <outfile1>:    e.g., C/VF_Host_L2_generated.h\n")
 
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

    qspecs_filename = argv [1]
    outputfile_name = argv [2]

    (qspecs_h2f, qspecs_f2h) = read_L2_MultiQueue_Spec (qspecs_filename)
    if debug:
        print_L2_MultiQueue_Specs (qspecs_h2f, qspecs_f2h)

    sys.stdout.write ('Generating "{:s}" ...\n'.format (outputfile_name))
    with open (outputfile_name, "w") as fpo:
        fpo.write ("// THIS FILE IS GENERATED; DO NOT EDIT.\n")
        fpo.write ("\n")
        gen_init_all_queues (fpo, qspecs_h2f, qspecs_f2h)
    sys.stdout.write ("... done\n")

    return 0

# ****************************************************************

def gen_init_all_queues (fpo, qspecs_h2f, qspecs_f2h):
    h2f_n_queues = len (qspecs_h2f)
    f2h_n_queues = len (qspecs_f2h)

    fpo.write ("static const int h2f_n_queues = {:d};\n".format (h2f_n_queues))
    fpo.write ("static Queue h2f_queues [{:d}];\n".format (h2f_n_queues))
    fpo.write ("\n")
    fpo.write ("static const int f2h_n_queues = {:d};\n".format (f2h_n_queues))
    fpo.write ("static Queue f2h_queues [{:d}];\n".format (f2h_n_queues))
    fpo.write ("\n")

    fpo.write ('static\n')
    fpo.write ('void init_all_queues ()\n')
    fpo.write ('{\n')

    for qspec in qspecs_h2f:
        fpo.write ('    init_queue (')
        fpo.write ('"h2f", {:d}, {:d}, {:d}, & (h2f_queues [{:d}])'
                   .format (qspec ["width_B"],
                            qspec ["capacity_h_I"],
                            qspec ["capacity_f_I"],
                            qspec ["id"]))
        fpo.write (');\n')

    for qspec in qspecs_f2h:
        fpo.write ('    init_queue (')
        fpo.write ('"f2h", {:d}, {:d}, {:d}, & (f2h_queues [{:d}])'
                   .format (qspec ["width_B"],
                            qspec ["capacity_f_I"],
                            qspec ["capacity_h_I"],
                            qspec ["id"]))
        fpo.write (');\n')

    fpo.write ('}\n')

# ****************************************************************
# ****************************************************************
# ****************************************************************
# For non-interactive invocations, call main() and use its return value
# as the exit code.
if __name__ == '__main__':
  sys.exit (main (sys.argv))

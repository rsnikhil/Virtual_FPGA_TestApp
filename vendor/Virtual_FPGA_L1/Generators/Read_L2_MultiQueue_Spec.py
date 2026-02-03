# Read an L2 MultiQueue Spec file, check for validity, and return the
# h2f and f2h descrs in order.

# ****************************************************************
# Import standard libs

import sys
import json

# ****************************************************************

def read_L2_MultiQueue_Spec (filename):

    sys.stdout.write ("Opening JSON file: {:s}\n".format(filename))
    with open (filename, 'r') as fpi:
        qspecs = json.load (fpi)

    qspecs_h2f = []
    qspecs_f2h = []
    for qspec in qspecs:
        if qspec ["dir"] == "h2f":
            qspecs_h2f.append (qspec)
        elif qspec ["dir"] == "f2h":
            qspecs_f2h.append (qspec)
        else:
            sys.stdout.write ('ERROR: unrecognized "dir" in queue spec\n'
                              .format (j, name))
            print (qspec)
            sys.stdout.write ('  Should be "h2f" or "f2h" only\n')
            sys.exit (1)

    # TODO: More checking that each spec is well-formed

    key_fun = lambda qspec: qspec ["id"]
    qspecs_h2f.sort (key = key_fun)
    qspecs_f2h.sort (key = key_fun)

    for j in range (len (qspecs_h2f)):
        qspec = qspecs_h2f [j]
        if (j != qspec ["id"]):
            sys.stdout.write ('ERROR: h2f qspec id is not consecutive\n')
            print (qspec)
            sys.exit (1)

    for j in range (len (qspecs_f2h)):
        qspec = qspecs_f2h [j]
        if (j != qspec ["id"]):
            sys.stdout.write ('ERROR: f2h qspec id is not consecutive\n')
            print (qspec)
            sys.exit (1)

    return (qspecs_h2f, qspecs_f2h)

def print_qspec (qspec):
    sys.stdout.write (" dir:{:s}".format (qspec ["dir"]))
    sys.stdout.write (" id:{:d}".format (qspec ["id"]))
    sys.stdout.write (" width_B:{:2d}".format (qspec ["width_B"]))
    sys.stdout.write (" capacity_h_I:{:2d}".format (qspec ["capacity_h_I"]))
    sys.stdout.write (" capacity_f_I:{:2d}\n".format (qspec ["capacity_f_I"]))

def print_L2_MultiQueue_Specs (qspecs_h2f, qspecs_f2h):
    sys.stdout.write ("h2f queues:\n")
    for j in range (len (qspecs_h2f)):
        sys.stdout.write ("  {:0d}:".format (j))
        print_qspec (qspecs_h2f [j])

    sys.stdout.write ("f2h queues:\n")
    for j in range (len (qspecs_f2h)):
        sys.stdout.write ("  {:0d}:".format (j))
        print_qspec (qspecs_f2h [j])

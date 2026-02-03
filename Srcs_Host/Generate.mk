# Makefile to generate:
#
#   VF_Host_L2_generated.h
#     This is #include'd by VF_Host_L2.c

.PHONY: help
help:
	@echo "Targets:"
	@echo "  all, gen    From  SPEC_FILE"
	@echo "              generates  GENERATED_FILES"
	@echo "where:"
	@echo "  SPEC_FILE ="
	@echo "    $(SPEC_FILE)"
	@echo "  GENERATED_FILES ="
	@echo "    $(GENERATED_FILE)"
	@echo "These can be overridden on 'make' command line"

# TODO: this is temporary; should point to copy of Virtual_FPGA repo
#       in this-repo's "vendor" area
VF_DIR = $(HOME)/Git/Virtual_FPGA_L1

# Python program that generates the files
GEN_VF_HOST_L2_C = $(VF_DIR)/Generators/Gen_VF_Host_L2_C.py

# Args to the Python program
SPEC_FILE ?= ../App_VF_Spec.json

# File to be generated
GENERATED_FILE = VF_Host_L2_generated.h

# ================================================================
# Generate files

.PHONY: all gen
all gen: $(SPEC_FILE)
	$(GEN_VF_HOST_L2_C)  $(SPEC_FILE)  $(GENERATED_FILE)

# ================================================================

.PHONY: clean
clean:
	rm -r -f  *~  .*~

.PHONY: full_clean
full_clean: clean
	rm -r -f  $(GENERATED_FILE)

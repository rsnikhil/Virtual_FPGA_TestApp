# Makefile to generate:
#
#   VF_HW_L1_Params.bsv
#     This is imported by VF_HW_L1.bsv
#
#   VF_HW_L2.bsv
#     This implements the HW-side multi-queues
#
#   VF_HW_L2_IFC.bsv
#     This is the interface of VF_HW_L2, and is a module parameter to
#     the user's BSV App.  Interface contains sub-interfaces for
#     clocks, multi-queues and DDR access, and other resource access.

.PHONY: help
help:
	@echo "Targets:"
	@echo "  all, gen    From  SPEC_FILE  and TEMPLATE_L2"
	@echo "              generates  GENERATED_FILES"
	@echo "where:"
	@echo "  SPEC_FILE ="
	@echo "    $(SPEC_FILE)"
	@echo "  TEMPLATE_L2 ="
	@echo "    $(TEMPLATE_L2)"
	@echo "  GENERATED_FILES ="
	@echo "    $(GENERATED_FILES)"
	@echo "These can be overridden on 'make' command line"

# TODO: this is temporary; should point to copy of Virtual_FPGA repo
#       in this-repo's "vendor" area
VF_DIR = $(HOME)/Git/Virtual_FPGA_L1

# Python program that generates the files
GEN_VF_HW_L2_BSV = $(VF_DIR)/Generators/Gen_VF_HW_L2_bsv.py

# Args to the Python program
SPEC_FILE   ?= ../App_VF_Spec.json
TEMPLATE_L2 ?= $(VF_DIR)/VF_L2/Srcs_HW/VF_HW_L2_TEMPLATE.bsv

# Files to be generated
GENERATED_FILES = VF_HW_L1_Params.bsv  VF_HW_L2.bsv  VF_HW_L2_IFC.bsv

# ================================================================
# Generate files

.PHONY: all gen
all gen:
	$(GEN_VF_HW_L2_BSV)  $(SPEC_FILE)  $(TEMPLATE_L2)

# ================================================================

.PHONY: clean
clean:
	rm -r -f *~

.PHONY: full_clean
full_clean: clean
	rm -r -f $(GENERATED_FILES)

.PHONY: help
help:
	@echo "This Makefile is to build TestApp's host-side executable, simulation version."
	@echo "Top-level: 'main.c'."
	@echo "Invokes Virtual_FPGA_V1 libraries (in 'vendor/')."
	@echo ""
	@echo "Available targets:"
	@echo "  all           Compile and link executable $(EXECUTABLE)"
	@echo "  run           make all; then run it"
	@echo ""
	@echo "  clean         Remove temporary intermediate files"
	@echo "  full_clean    Restore to pristine state"

# ================================================================

EXECUTABLE = exe_VF_TestApp_Host_Sim

.PHONY: all
all: $(EXECUTABLE)

# ================================================================
# Sources for this app

APP_DIR = ../..
SRCS_C += $(APP_DIR)/Srcs_Host/main.c

# ================================================================
# External libs (under 'vendor')

VENDOR_DIR = ../../vendor
VF_DIR = $(VENDOR_DIR)/Virtual_FPGA_L1

VF_L2_DIR = $(VF_DIR)/VF_L2/Srcs_Host
VF_L1_DIR = $(VF_DIR)/VF_L1_Sim/Srcs_Host

SRCS_C += $(VF_L2_DIR)/VF_Host_L2.c

SRCS_C += $(VF_L1_DIR)/VF_Host_L1.c
SRCS_C += $(VF_L1_DIR)/TCP_Client_Lib.c

SRCS_H += $(VF_L2_DIR)/VF_Host_L2.h
SRCS_H += $(VF_L2_DIR)/VF_Host_L2_protos.h

SRCS_H += $(VF_L1_DIR)/VF_Host_L1.h
SRCS_H += $(VF_L1_DIR)/VF_Host_L1_protos.h
SRCS_H += $(VF_L1_DIR)/TCP_Client_Lib.h
SRCS_H += $(VF_L1_DIR)/TCP_Client_Lib_protos.h

# ================================================================
# Build (C-compile and link)

# CFLAGS = -g

$(EXECUTABLE): $(SRCS_H) $(SRCS_C)
	$(CC) $(CFLAGS) -o $(EXECUTABLE) \
	    -I $(VF_L2_DIR) \
	    -I $(VF_L1_DIR) \
	    $(SRCS_C)

.PHONY: run
run: $(EXECUTABLE)
	./$(EXECUTABLE)

# ================================================================
# .h files are extracted automatically from .c files
# using a Python script

%_protos.h: %.c
	C_Protos_Extract.py $<

# ****************************************************************

.PHONY: clean
clean:
	rm -r -f  *~  .*~

.PHONY: full_clean
full_clean: clean
	rm -r -f  exe_*  log*

# ****************************************************************

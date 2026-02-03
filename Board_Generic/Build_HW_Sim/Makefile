.PHONY: help
help:
	@echo "See documentation for HW-side build flow for context."
	@echo "Requires certain files to be generated, prior to this."
	@echo ""
	@echo "This Makefile is to build TestApp's HW-side executable, simulation version."
	@echo "The module hierarchy, in outline, is:"
	@echo "    mkVF_HW_Top       [1]"
	@echo "        mkApp_HW"
	@echo "        mkVF_HW_L2    [2]"
	@echo "[1] and [2] are in vendor/Virtual_HW_L1/"
	@echo ""
	@echo "Available targets:"
	@echo "  b_compile b_link         compile and link for Bluesim"
	@echo "  v_compile v_link         compile and link for Verilator"
	@echo ""
	@echo "  b_run  v_run             run the Bluesim or Verilator exe"
	@echo ""
	@echo "  b_all = b_compile  b_link"
	@echo "  v_all = v_compile  v_link"
	@echo ""
	@echo "  clean                    Remove temporary intermediate files"
	@echo "  full_clean               Restore to pristine state"

.PHONY: all
b_all: b_compile b_link
v_all: v_compile v_link

# ================================================================

# Note: builds will suffix the following with _bsim or _vsim
EXECUTABLE = exe_TestApp_HW_Sim

# ================================================================

# Root dir of this repo
ROOT_D = ../..

SRCS_APP_HW_D = $(ROOT_D)/Srcs_HW

# ================================================================
# External libs (under 'vendor')

VENDOR_D = ../../vendor

VENDOR_VIRT_HW_D = $(VENDOR_D)/Virtual_FPGA_L1

SRCS_L1_HW_D = $(VENDOR_VIRT_HW_D)/VF_L1_Sim/Srcs_HW
SRCS_L2_HW_D = $(VENDOR_VIRT_HW_D)/VF_L2/Srcs_HW

TOP_FILE   ?= $(SRCS_L2_HW_D)/VF_HW_Top.bsv
TOP_MODULE ?= mkVF_HW_Top

LIB_MISC_D += $(VENDOR_D)/bsc-contrib_Libraries/Misc
LIB_AXI4_D += $(VENDOR_D)/bsc-contrib_Libraries/AMBA_Fabrics/AXI4

# Suppress warning G0020 (unpredictable order of $display etc. in methods)
BSCFLAGS = \
	-use-dpi \
	-keep-fires \
	-aggressive-conditions \
	-no-warn-action-shadowing \
	-show-range-conflict \
        -opt-undetermined-vals \
	-unspecified-to X \
	-show-schedule \
        -suppress-warnings G0020

C_FILES  = $(SRCS_L1_HW_D)/C_Imported_Functions.c

# Only needed for imported C code
BSC_C_FLAGS += -Xl -v  -Xc -O3  -Xc++ -O3

# ----------------
# bsc's directory search path

BSCPATH = $(SRCS_L1_HW_D):$(SRCS_APP_HW_D):$(LIB_MISC_D):$(LIB_AXI4_D):+

# ****************************************************************
# FOR VERILATOR

VSIM      = verilator

BSCDIRS_V = -bdir build_v  -info-dir build_v  -vdir verilog

BSCPATH_V = $(BSCPATH)

build_v:
	mkdir -p $@

verilog:
	mkdir -p $@

.PHONY: v_compile
v_compile: $(VF_HW_L2_BSV) build_v verilog
	@echo "Compiling for Verilog (Verilog generation) ..."
	bsc -u -elab -verilog  $(BSCDIRS_V)  $(BSCFLAGS)  -p $(BSCPATH_V)  $(TOP_FILE)
	@echo "Verilog generation finished"

.PHONY: v_link
v_link: build_v verilog
	@echo "Linking for Verilog simulation (simulator: $(VSIM)) ..."
	bsc -verilog  -vsim $(VSIM)  -use-dpi  -keep-fires  -v  $(BSCDIRS_V) \
		-e $(TOP_MODULE) -o ./$(EXECUTABLE)_$(VSIM) \
		$(BSC_C_FLAGS) \
		$(C_FILES)
	@echo "Linking for Verilog simulation finished"

# ----------------
# Verilator runs

.PHONY: v_run
v_run:
	@echo "INFO: Simulation ..."
	./$(EXECUTABLE)_$(VSIM)
	@echo "INFO: Finished Simulation"

# ****************************************************************
# FOR BLUESIM

BSCDIRS_BSIM_c = -bdir build_b -info-dir build_b
BSCDIRS_BSIM_l = -simdir C_for_bsim

BSCPATH_BSIM = $(BSCPATH)

build_b:
	mkdir -p $@

C_for_bsim:
	mkdir -p $@

.PHONY: b_compile
b_compile: $(VF_HW_L2_BSV) build_b
	@echo Compiling for Bluesim ...
	bsc -u -sim $(BSCDIRS_BSIM_c)  $(BSCFLAGS)  -p $(BSCPATH_BSIM)  $(TOP_FILE)
	@echo Compilation for Bluesim finished

.PHONY: b_link
b_link: build_b C_for_bsim
	@echo Linking for Bluesim ...
	bsc  -sim  -parallel-sim-link 8\
		$(BSCDIRS_BSIM_c)  $(BSCDIRS_BSIM_l)  -p $(BSCPATH_BSIM) \
		-e $(TOP_MODULE) -o ./$(EXECUTABLE)_bsim \
		-keep-fires \
		$(BSC_C_FLAGS)  $(C_FILES)
	@echo Linking for Bluesim finished

# ----------------

.PHONY: b_run
b_run:
	@echo "INFO: Simulation ..."
	./$(EXECUTABLE)_bsim
	@echo "INFO: Finished Simulation"

# ****************************************************************

.PHONY: clean
clean:
	rm -r -f  *~  .*~  src_*/*~  build*  C_for_bsim

.PHONY: full_clean
full_clean: clean
	rm -r -f  exe_*  verilog  $(L1_C_D)/*.o  BSV/*
	rm -r -f  $(VF_HW_L1_PARAMS)  $(VF_HW_L2_BSV)
	rm -r -f  log*  build_b  obj_dir_*

# ****************************************************************

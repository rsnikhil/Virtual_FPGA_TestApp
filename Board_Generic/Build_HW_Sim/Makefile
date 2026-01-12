.PHONY: help
help:
	@echo "This Makefile is to build TestApp's HW-side executable, simulation version."
	@echo "The module hierarchy, in outline, is:"
	@echo "    mkVirtual_HW_L1_Sim     [1]"
	@echo "        mkVirtual_HW_APP_HW    [2]"
	@echo "[1] file vendor/Virtual_HW_L1/Sim_Lib/Srcs_HW/Srcs_BSV/VF_Sim.bsv"
	@echo "[2] file Srcs_HW/Srcs_BSV/Virtual_HW_APP_HW.bsv"
	@echo ""
	@echo "Available targets:"
	@echo "  b_compile b_link         bsc-compile and link an executable for Bluesim"
	@echo "  v_compile v_link         bsc-compile and link an executable for Verilator"
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
# Sources for this App

APP_DIR         = ../..
SRCS_APP_HW_DIR = $(APP_DIR)/Srcs_HW

# ================================================================
# External libs (under 'vendor')

VENDOR_DIR = ../../vendor

VENDOR_VIRT_HW_DIR = $(VENDOR_DIR)/Virtual_FPGA_L1

SRCS_L1_HW_DIR = $(VENDOR_VIRT_HW_DIR)/VF_L1_Sim/Srcs_HW
SRCS_L2_HW_DIR = $(VENDOR_VIRT_HW_DIR)/VF_L2/Srcs_HW

TOP_FILE   ?= $(SRCS_L2_HW_DIR)/VF_HW_Top.bsv
TOP_MODULE ?= mkVF_HW_Top

LIB_MISC_DIR += $(VENDOR_DIR)/bsc-contrib_Libraries/Misc
LIB_AXI4_DIR += $(VENDOR_DIR)/bsc-contrib_Libraries/AMBA_Fabrics/AXI4

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

C_FILES  = $(SRCS_L1_HW_DIR)/C_Imported_Functions.c

# Only needed for imported C code
BSC_C_FLAGS += -Xl -v  -Xc -O3  -Xc++ -O3

# ----------------
# bsc's directory search path

BSCPATH = $(SRCS_L1_HW_DIR):$(SRCS_L2_HW_DIR):$(SRCS_APP_HW_DIR):$(LIB_MISC_DIR):$(LIB_AXI4_DIR):+

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
v_compile: build_v verilog
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
b_compile: build_b
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
	rm -r -f  exe_*  verilog  log*  $(L1_C_DIR)/*.o  build_b  obj_dir_*

# ****************************************************************

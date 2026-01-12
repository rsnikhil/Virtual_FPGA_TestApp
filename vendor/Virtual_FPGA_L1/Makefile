# Makefile.

BSC=bsc
STEPS=100
STEPS_WARN_INTERVAL=1000000
STEPS_MAX_INTERVALS=1000000
BSC_COMP_FLAGS = -elab -keep-fires -aggressive-conditions -no-warn-action-shadowing -show-range-conflict +RTS -K8M --RTS \
		-suppress-warnings G0010 -suppress-warnings G0020 -suppress-warnings S0080 \
		-steps $(STEPS) \
		-steps-warn-interval $(STEPS_WARN_INTERVAL) \
		-steps-max-intervals $(STEPS_MAX_INTERVALS)

BSC_LINK_FLAGS =

BSC_C_FLAGS += \
	-Xc -std=c18 \
	-Xc++ -D_GLIBCXX_USE_CXX11_ABI=1 \
	-Xl -v \
	-Xc -O3 -Xc++ -O3 \
#	-Xc -g -Xc++ -g \

TOP ?= TbVirtual_FPGA_DUT
TOPMODULE = mk$(TOP)
TOPFILE   = test/$(TOP).bsv

EXE_BSIM = ./bin/exe_bsim

VCDVIEWER = gtkwave

BDPI_C_SRCS =
EXTRA_BSV_SRCS = ./src/bsv/common:./src/bsv/virtual_fpga:

.PHONY: default
default: compile link

.PHONY: help
help:
	@echo "Possible targets:"
	@echo "  compile       --  compile $(TOPFILE) for the bluesim"
	@echo "  link          --  link compiled files for bluesim"
	@echo "  simulate      --  compile, link, and simulate using bluesim"
	@echo "  v_compile     --  compile $(TOPFILE) for verilog"
	@echo "  v_link        --  link compiled files for verilog simulation"
	@echo "  v_simulate    --  compile, link, and simulate using iverilog"
	@echo "  clean         --  remove binary, executable and temporary files"
	@echo "  full_clean    --  remove all generated folders and files"
	@echo "  help          --  display this message"

# ----------------------------------------------------------------
# Bluesim compile/link/simulate

BSCDIRS_BSIM  = -simdir build/bsim -bdir build/bsim -info-dir build/info
BSCPATH_BSIM  = -p .:./src/bsv:$(EXTRA_BSV_SRCS):%/Libraries
BSCLINKFLAGS_BSIM = -keep-fires

.PHONY: compile
compile:
	@echo Compiling for Bluesim ...
	mkdir -p build/bsim build/info bin
	$(BSC) -u -sim $(BSCDIRS_BSIM) $(BSC_COMP_FLAGS) $(BSCPATH_BSIM) -g $(TOPMODULE) $(TOPFILE)
	@echo Compiling for Bluesim finished

.PHONY: link
link:   compile
	@echo Linking for Bluesim ...
	$(BSC) -sim  -parallel-sim-link 8 \
		$(BSCDIRS_BSIM)  $(BSCPATH_BSIM) \
		-e $(TOPMODULE)  -o $(EXE_BSIM)  \
		$(BSCLINKFLAGS_BSIM) \
		$(BSC_C_FLAGS) \
		$(BDPI_C_SRCS)
	@echo Linking for Bluesim finished

.PHONY: simulate
simulate: link
	@echo Bluesim simulation ...
	$(EXE_BSIM) +bscvcd -V
	# $(VCDVIEWER)  dump.vcd ./config/signal-view.sav &
	@echo Bluesim simulation finished

# ----------------------------------------------------------------
# Verilog compile/link/sim

BSCDIRS_V = -vdir src/v -bdir build/v -info-dir build/info
BSCPATH_V = -p .:./src/bsv:$(EXTRA_BSV_SRCS):%/Libraries

# Set VSIM to desired Verilog simulator
VSIM = iverilog

EXE_VSIM = ./bin/exe_vsim

.PHONY: v_compile
v_compile:
	@echo Compiling for Verilog ...
	mkdir -p src/v build/v build/info bin
	$(BSC) -u -verilog -elab $(BSCDIRS_V) $(BSC_COMP_FLAGS) $(BSCPATH_V) -g $(TOPMODULE) $(TOPFILE)
	@echo Compiling for Verilog finished

.PHONY: v_link
v_link: v_compile
	@echo Linking for Verilog simulation ...
	bsc -e $(TOPMODULE) -verilog -o $(EXE_VSIM) $(BSCDIRS_V) -vsim $(VSIM) -keep-fires \
		src/v/$(TOPMOD).v \
		build/v/*.ba \
		$(BDPI_C_SRCS)
	@echo Linking for Verilog simulation finished

.PHONY: v_simulate
v_simulate: v_link
	@echo Verilog simulation ...
	$(EXE_VSIM) +bscvcd
	# $(VCDVIEWER)  dump.vcd ./config/signal-view.sav &
	@echo Verilog simulation finished

# ----------------------------------------------------------------

.PHONY: clean
clean:
	rm -f ./*~ ./*.sft ./*.so ./*.vcd ./bin/* src/bsv/*~ ./src/v/*~ build/bsim/* build/v/* build/info/* src/c/*.o

.PHONY: full_clean
full_clean:
	rm -rf transcript*
	rm -rf ./*~ ./*.sft ./*.so ./*.vcd ./bin ./src/bsv/*~ ./src/v ./src/c/*.o ./build


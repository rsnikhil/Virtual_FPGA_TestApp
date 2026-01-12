.PHONY: help
help:
	@echo "Targets:"
	@echo "  $(TARGETS)"
	@echo "  all           All targets"
	@echo "  clean         Delete temporary files"
	@echo "  full_clean    Restore to pristine state"
	@echo "See each <target>.vendor.hjson for details of remote repos etc."

.PHONY: all
all:
	make  $(TARGETS)

# ================================================================
VENDOR_PY = ./vendor.py

# ================================================================
TARGETS += bsc-contrib_Libraries
LOCKS   += bsc-contrib_Libraries.lock.hjson
.PHONY: bsc-contrib_Libraries
bsc-contrib_Libraries:
	$(VENDOR_PY)  bsc-contrib_Libraries.vendor.hjson

# ================================================================
TARGETS += Virtual_FPGA_L1
LOCKS   += Virtual_FPGA_L1.lock.hjson
.PHONY: Virtual_FPGA_L1
Virtual_FPGA_L1:
	$(VENDOR_PY)  Virtual_FPGA_L1.vendor.hjson

# ================================================================

.PHONY: clean
clean:
	rm -r -f  *~

.PHONY: full_clean
full_clean: clean
	rm -r -f  $(TARGETS)  $(LOCKS)

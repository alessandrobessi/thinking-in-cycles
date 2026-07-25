# Top-level developer entry points for "Thinking in Cycles".
# See ROADMAP.md for what's implemented vs. pending.

.DEFAULT_GOAL := help

.PHONY: help doctor lab-cyclelab cyclelab-debug cyclelab-release smoke clean

help: ## list available targets
	@echo "Targets:"
	@echo "  make doctor          report this machine's lab-environment capabilities"
	@echo "  make lab-cyclelab    build cyclelab (debug + release)"
	@echo "  make smoke           build cyclelab and run a minimal functional check"
	@echo "  make clean           remove build artifacts"

doctor: ## run the environment doctor
	@bash scripts/doctor.sh

lab-cyclelab: cyclelab-debug cyclelab-release ## build both cyclelab variants

cyclelab-debug:
	$(MAKE) -C labs/cyclelab debug

cyclelab-release:
	$(MAKE) -C labs/cyclelab release

smoke: lab-cyclelab ## build cyclelab and smoke-test the compute mode
	@bash scripts/smoke_test_labs.sh

clean: ## remove build artifacts
	$(MAKE) -C labs/cyclelab clean

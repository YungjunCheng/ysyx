# Makefile for Verilator simulation

# Default target
all: run

# Verilator configuration
VERILATOR = verilator
VERILATOR_FLAGS = --cc --exe --build -j 0 -Wall
SOURCES = our.v sim_main.cpp

# Build the simulation
sim: $(SOURCES)
	$(VERILATOR) $(VERILATOR_FLAGS) sim_main.cpp our.v

# Run the simulation
run: sim
	@echo "Running simulation..."
	./obj_dir/Vour

# Clean up generated files
clean:
	rm -rf obj_dir *.log *.vcd

.PHONY: all sim run clean


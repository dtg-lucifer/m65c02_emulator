default: all
.PHONY: default

all: build run
.PHONY: all

build:
	$(MAKE) -C build \
		-j 12 \
        --no-print-directory
.PHONY: build

run:
	@./build/bin/6502_cpu_emulator
.PHONY: run

setup:
	@mkdir -p build && \
		cmake -S . -B build \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON && \
        ln -sf build/compile_commands.json compile_commands.json
.PHONY: setup

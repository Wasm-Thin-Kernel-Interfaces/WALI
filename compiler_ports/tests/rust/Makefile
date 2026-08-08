# Integration tests for the `wasm32-wali-linux-musl` rustc target.
#
# The `wali` toolchain (`make rustc` at the repo root) provides the compiler;
# the target triple and the iwasm runner come from .cargo/config.toml.

CARGO ?= cargo +wali
CARGO_FLAGS ?=

.PHONY: all build test clean

all: build

build:
	$(CARGO) build --tests $(CARGO_FLAGS)

test:
	$(CARGO) test $(CARGO_FLAGS)

clean:
	$(CARGO) clean

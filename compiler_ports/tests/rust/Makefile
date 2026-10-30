# Integration tests for the `wasm32-wali-linux-musl` rustc target.
#
# Upstream nightly (`make rustc` at the repo root) provides the compiler;
# the target triple, build-std and the iwasm runner come from .cargo/config.toml.

CARGO ?= cargo +nightly
CARGO_FLAGS ?=

# Link against the WALI sysroot (crt objects, libc.a and the empty libunwind.a)
WALI_SYSROOT_LIB ?= $(abspath ../../../build/sysroot/lib)
export CARGO_TARGET_WASM32_WALI_LINUX_MUSL_RUSTFLAGS ?= -L native=$(WALI_SYSROOT_LIB)

.PHONY: all build test clean

all: build

build:
	$(CARGO) build --tests $(CARGO_FLAGS)

test:
	$(CARGO) test $(CARGO_FLAGS)

clean:
	$(CARGO) clean

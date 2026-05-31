include ../toolchains/wali.mk

.PHONY: dir clean all test native wasm rebuild-wasm

BIN_DIR := bin
WASM_DIR := $(BIN_DIR)/unit/wasm
ELF_DIR := $(BIN_DIR)/unit/elf

# Flags
CFLAGS_TEST_WASM = $(WALI_COMMON_CFLAGS) $(WALI_COMMON_LDFLAGS) -I. -Icommon -nostartfiles -Wl,--no-entry -Wl,--allow-undefined
CFLAGS_TEST_NATIVE = -I. -Icommon -O2 -Wno-unused-result

IWASM_ROOT = ../wasm-micro-runtime

SRC_C := $(wildcard unit/*.c)

SRC_TESTS := $(filter-out %_hooks.c, $(SRC_C))

SRC_WASM := $(patsubst unit/%.c, $(WASM_DIR)/%.wasm, $(SRC_TESTS))
SRC_ELF := $(patsubst unit/%.c, $(ELF_DIR)/%, $(SRC_TESTS))
BIN_HOOKS := $(patsubst unit/%.c, $(ELF_DIR)/%_hooks, $(SRC_TESTS))

all: wasm native

native: $(SRC_ELF) $(BIN_HOOKS)

wasm: $(SRC_WASM)

rebuild-wasm:
	rm -f $(SRC_WASM) $(SRC_WASM:.wasm=.wat)
	$(MAKE) wasm

$(ELF_DIR)/%_hooks: unit/%.c common/hooks_main.c
	@mkdir -p $(dir $@)
	gcc $(CFLAGS_TEST_NATIVE) -DWALI_TEST_WRAPPER -o $@ common/hooks_main.c $<

$(WASM_DIR)/%.wasm: unit/%.c
	@mkdir -p $(dir $@)
	$(WALI_CC) $(CFLAGS_TEST_WASM) -o $@ $<
	wasm2wat --enable-threads $@ -o $(@:.wasm=.wat)

$(ELF_DIR)/%: unit/%.c
	@mkdir -p $(dir $@)
	gcc $(CFLAGS_TEST_NATIVE) -o $@ $< -lm -lpthread

clean:
	rm -rf $(BIN_DIR)

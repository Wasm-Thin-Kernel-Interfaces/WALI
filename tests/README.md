# WALI Test Suite

This folder contains the test suite for end-to-end testing of WALI ecosystems using differential testing against a native ELF binary.

## Running tests

```bash
make all                                  # build all three artifacts per test
python3 run_tests.py                      # full suite
python3 run_tests.py -f open chmod        # subset
python3 run_tests.py -v                   # verbose, prints all output
python3 run_tests.py --config other.json  # alternate engine config
```

### Engine config

`--config` points at a JSON list — every entry is a WASM engine the suite runs each test against. Default is [engines.json](engines.json):

```json
[
  {
    "name": "iwasm",
    "command": "../iwasm",
    "args": ["{verbose_arg}", "--env-file={env_file}", "{wasm_file}", "{args}"],
    "verbose_arg": "-v=5"
  }
]
```

- `command` — engine binary.
- `args` — argv template. Supported placeholders: `{wasm_file}` (the `.wasm` path), `{env_file}` (per-run env file written by the runner), `{verbose_arg}` (substituted with `verbose_arg` only when `-v` is passed; empty otherwise), `{args}` (test args from `// CMD: args=...`; if omitted from the template, test args are appended at the end).
- `verbose_arg` — engine-specific verbose flag.

## Anatomy of a unit test

A test must:

1. `#include "wali_start.c"`.
2. Define `int test(void)` returning 0 on success, non-zero on failure. Pass/fail is signaled via the return value — printed output is not compared.

Optionally:

- **`// CMD:` directives** at the top of the file drive parameterized runs. Each line produces one full setup → test → cleanup cycle. Recognized keys (shell-quoted): `setup="..."`, `args="..."`, `cleanup="..."`, `env="K=V K2=V2"`. If `cleanup=` is omitted it defaults to `setup=` args.
- **`test_setup(argc, argv)` / `test_cleanup(argc, argv)`** inside `#ifdef WALI_TEST_WRAPPER`. The guard keeps libc-heavy fixture code out of the WASM build while still compiling it into the hooks wrapper.
- **`test_init_args()`** at the start of `test()` if you read `argv` — required under WASM to populate argc/argv from WALI imports (no-op natively).

**`argv` indexing — note the asymmetry between `test()` and `test_setup`/`test_cleanup`:**

- Inside `test()`, `argv` follows the standard C convention. `argv[0]` is the program path (the test binary or the `.wasm` file); user args from `// CMD: args=...` start at `argv[1]`. So `argc >= 1 + N` where `N` is the number of user args.
- Inside `test_setup` / `test_cleanup`, the hooks wrapper has already stripped the program path and the `setup`/`cleanup` sub-command before invoking the hook. `argv[0]` is the **first** user arg from `// CMD: setup=...` / `cleanup=...`. So `argc == N` exactly.

This means a `// CMD: args="ok 0777 /tmp/f"` line is read inside `test()` as `argv[1]="ok"`, `argv[2]="0777"`, `argv[3]="/tmp/f"`, but a `// CMD: setup="0644 /tmp/f"` line is read inside `test_setup` as `argv[0]="0644"`, `argv[1]="/tmp/f"`.

See [unit/chmod.c](unit/chmod.c) for a representative example, or [unit/open.c](unit/open.c) for one that uses `// CMD:` directives to parameterize a single `test()` over multiple runs.

## Architecture

Every test in [unit/](unit/) compiles from a single `.c` source into **both** a native ELF binary and a WASM module. [run_tests.py](run_tests.py) runs both and compares exit codes — divergence means WALI exposes different semantics under WASM than the host kernel does natively.

The harness in [common/](common/) is what makes one source file work for both targets:

- **[wali_start.c](common/wali_start.c)** — included as a header by every test. Emits `_start` under `__wasm__`, `main` otherwise. Suppressed when `WALI_TEST_WRAPPER` is defined so the same source can also be linked into the hooks binary.
- **[hooks_main.c](common/hooks_main.c)** — provides `main()` for a native-only `<name>_hooks` wrapper that runs `test_setup` / `test_cleanup` (defined as weak symbols, optionally overridden by the test). The runner invokes this wrapper before/after the test to manage fixtures.
- **[wali_syscall_utils.h](common/wali_syscall_utils.h)** — `wali_syscall_*` wrappers that bind to WALI imports under WASM and to libc/raw syscalls natively. The seam that lets test bodies be target-agnostic.
- **[wali_test_helpers.h](common/wali_test_helpers.h)** — `TEST_ASSERT_EQ` / `TEST_ASSERT_NE` / `TEST_LOG` macros that write directly via `wali_syscall_write` so they work identically under WASM (no libc dependency).

The [Makefile](Makefile) builds three artifacts per `unit/<name>.c`: `bin/unit/wasm/<name>.wasm`, `bin/unit/elf/<name>`, and `bin/unit/elf/<name>_hooks` (the last with `-DWALI_TEST_WRAPPER`).

> **Known WALI Issues in Test Suite to Fix**
> - **[unit/mmap.c](unit/mmap.c) — `bad_size_zero`**: native `mmap(NULL, 0, ...)` returns `-1` with `EINVAL`; WALI returns a valid pointer (size-0 mmap silently accepted).
> - **[unit/mremap.c](unit/mremap.c) — `bad_addr`**: the test passes a high address (`0x100000000`) to exercise mremap-on-unmapped. That value isn't representable in 32-bit WASM linear memory, so the pointer is truncated/translated and the call doesn't fail the way native mremap does.
> - **[unit/prctl.c](unit/prctl.c) — `set_get_name`, `get_pdeathsig`**: prctl takes variable arguments with context-dependent pointer conversion on some args. Not supported yet, but support it soon.

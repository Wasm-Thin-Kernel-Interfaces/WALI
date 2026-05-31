#!/bin/bash
# Quick setup for WALI: generate toolchain configs and build components.
# Default builds runtime, compiler, and libc. Use -r to build only the runtime.
#
# `set -e` causes the script to abort immediately if any command fails.

set -e

usage() {
    cat <<EOF
Usage: $0 [-r|--runtime-only] [-d|--debug] [-h|--help]

Generates toolchain configs and builds WALI components.

Options:
  -r, --runtime-only   Only build the runtime (iwasm)
  -d, --debug          Build iwasm in Debug mode (MODE=Debug)
  -h, --help           Show this help message

Default: full setup (runtime + compiler + libc), iwasm in Release mode
EOF
}

RUNTIME_ONLY=0
DEBUG=0
while [ $# -gt 0 ]; do
    case "$1" in
        -r|--runtime-only) RUNTIME_ONLY=1 ;;
        -d|--debug) DEBUG=1 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage; exit 1 ;;
    esac
    shift
done

cd "$(dirname "$0")"

if [ "$DEBUG" -eq 1 ]; then
    IWASM_MODE_ARG="MODE=Debug"
    IWASM_MODE_LABEL=" (Debug)"
else
    IWASM_MODE_ARG=""
    IWASM_MODE_LABEL=""
fi

echo "==> Generating toolchain configs"
python3 toolchains/gen_toolchains.py

echo "==> Building runtime (iwasm)${IWASM_MODE_LABEL}"
git submodule update --init wasm-micro-runtime
make iwasm $IWASM_MODE_ARG

if [ "$RUNTIME_ONLY" -eq 1 ]; then
    echo "==> Runtime-only setup complete"
    exit 0
fi

echo "==> Building compiler (LLVM, slim)"
make compiler SLIM=1

echo "==> Building libc (wali-musl)"
git submodule update --init wali-musl
make libc

echo "==> Full setup complete"

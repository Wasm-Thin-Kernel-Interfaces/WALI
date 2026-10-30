#!/bin/bash
#
# Bundle the compiler-rt builtins from wasi-sdk with the WALI rt into the LLVM builtin
set -e
source $(dirname $(realpath ${BASH_SOURCE}))/../wali.sh

# Use the wasi-sdk release built with the same LLVM major version (wasi-sdk-34 is LLVM 23)
WASI_SDK_VERSION=34
LLVM_VERSION=$WALI_LLVM_MAJOR_VERSION
llvm_builtin=llvm-${LLVM_VERSION}.libclang_rt.builtins-wasm32-wali.a

curl -fL https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VERSION}/libclang_rt-${WASI_SDK_VERSION}.0.tar.gz \
	| tar -xzO libclang_rt-${WASI_SDK_VERSION}.0/wasm32-unknown-wasi/libclang_rt.builtins.a > $llvm_builtin

$WALI_CC $WALI_COMMON_CFLAGS -c wali_rt.c -o wali_rt.o
$WALI_LLVM_BIN_DIR/llvm-ar rcs $llvm_builtin wali_rt.o
rm wali_rt.o

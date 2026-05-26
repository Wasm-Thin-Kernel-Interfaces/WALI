# wali

Syscall definitions and ABI type system for the **WebAssembly Linux Interface
(WALI)**.

This package is the machine-readable source of truth for all syscalls and auxillary methods (with arguments types, names, and per-architecture Linux syscall numbers).
It also contains the **ABI** with exact
byte-level layouts of all relevant types.

This package is intended for building WALI
spec-related tooling: code generators, bindings, validators, documentation, etc.

## Install

```sh
pip install wali
```

## Usage

```python
from wali.spec import SYSCALLS, AUX_SYSCALLS, TypeRegistry

# Syscall metadata
read = SYSCALLS["read"]
read.nr                 # 0  (x86_64 number)
read.nrs.arm64          # 63
read.args               # ['int', 'void*', 'size_t']
read.args_id            # ['fd', 'buf', 'count']
read.implemented        # True

# ABI type layout
TypeRegistry.resolve_size("struct stat")       # 144
TypeRegistry.resolve_primitive("off_t")         # Primitive(size=8, signed=True)

stat = TypeRegistry.struct_defs["struct stat"]
for f, off, sz in zip(stat.fields, stat.field_offsets, stat.field_sizes):
    print(f"+{off:>3}  {f.name:<12} {f.type_name:<16} ({sz} bytes)")
```

### Public API

Everything is importable from the `wali.spec` namespace.

| Name | Description |
|------|-------------|
| `SYSCALLS` | `dict[str, Syscall]` of all defined Linux syscalls |
| `AUX_SYSCALLS` | `dict[str, AuxSyscall]` of WALI auxiliary calls |
| `Syscall`, `AuxSyscall`, `ArchNrs`, `SyscallArg` | syscall dataclasses |
| `TypeRegistry` | ABI type system: primitives, aliases, arrays, structs, and layout resolution |
| `Primitive`, `ArrayType`, `StructField` | type-system dataclasses |

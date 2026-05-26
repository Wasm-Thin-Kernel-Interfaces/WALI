"""The WALI specification: syscall definitions and the ABI type system.

This is the public namespace for the package. Everything is importable here:

>>> from wali.spec import SYSCALLS, TypeRegistry
>>> SYSCALLS["read"].nrs.x86_64
0
>>> TypeRegistry.resolve_size("struct stat")
144
"""

from .abi import (
    TypeRegistry,
    Primitive,
    ArrayType,
    StructField,
)
from .syscalls import (
    SYSCALLS,
    AUX_SYSCALLS,
    Syscall,
    AuxSyscall,
    ArchNrs,
    SyscallArg,
)

__all__ = [
    # Type system
    "TypeRegistry",
    "Primitive",
    "ArrayType",
    "StructField",
    # Syscall definitions
    "SYSCALLS",
    "AUX_SYSCALLS",
    "Syscall",
    "AuxSyscall",
    "ArchNrs",
    "SyscallArg",
]

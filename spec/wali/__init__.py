"""WALI (WebAssembly Linux Interface).

The specification — syscall definitions and the ABI type system — lives in the
:mod:`wali.spec` namespace:

>>> from wali.spec import SYSCALLS, TypeRegistry
>>> SYSCALLS["read"].nrs.x86_64
0
>>> TypeRegistry.resolve_size("struct stat")
144
"""

from . import spec

__version__ = "0.2.0"

__all__ = ["spec", "__version__"]

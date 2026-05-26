"""
abi.py - Source of truth for WALI ABI type definitions.

Defines the exact byte-level layout of all types used in WALI syscall interfaces.
All type sizes, alignments, and struct field offsets are either explicitly defined
or computed from C alignment rules.

Usage:
    from wali.spec import TypeRegistry

    size = TypeRegistry.resolve_size("struct stat")   # -> 144
    prim = TypeRegistry.resolve_primitive("off_t")     # -> Primitive(8, True)
"""

from __future__ import annotations
from dataclasses import dataclass, field

import re
import logging

# =============================================================================
# Dataclass Definitions
# =============================================================================

@dataclass(frozen=True)
class Primitive:
    """A primitive type with known size and signedness."""
    size: int       # bytes: 1, 2, 4, or 8
    signed: bool

    @property
    def wit_name(self) -> str:
        """Compute WIT type name from size and signedness."""
        return f"{'s' if self.signed else 'u'}{self.size * 8}"


@dataclass(frozen=True)
class ArrayType:
    """A named fixed-size array type (e.g., sigset_t = uint8_t[128])."""
    element_type: str   # c_primitives key
    count: int          # number of elements


@dataclass(frozen=True)
class StructField:
    """A field in a struct definition."""
    name: str           # e.g., "st_dev", "pad0"
    type_name: str      # e.g., "dev_t", "uint32_t", "long[3]"
    offset: int | None = None   # Optional explicit byte offset override.
                                # If None: computed from C alignment rules.
                                # If set: used as-is (must not overlap previous field).


# =============================================================================
# TypeRegistry
# =============================================================================

class TypeRegistry:
    """WALI ABI type system. Single source of truth for all type data and resolution.

    This is a singleton — do not instantiate. Use class attributes and classmethods directly.
    """
    _instance = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
        return cls._instance

    # Regex for parsing inline array notation: "long[3]", "unsigned long[4]"
    _INLINE_ARRAY_RE = re.compile(r'^(.+?)\[(\d+)\]$')

    # --- Type Data (class-level) ---
    # Naming convention: spaces for multi-word C types (unsigned int, long long, struct stat),
    # underscores only where they're part of the actual C identifier (off_t, uint32_t, ptr_func).

    c_primitives: dict[str, Primitive] = {
        # Standard C types (64-bit Linux sizes)
        "int":                Primitive(4, True),
        "long":               Primitive(8, True),
        "long long":          Primitive(8, True),
        "short":              Primitive(2, True),
        "reg":                Primitive(8, True),
        "unsigned":           Primitive(4, False),
        "unsigned int":       Primitive(4, False),
        "unsigned long":      Primitive(8, False),
        "unsigned long long": Primitive(8, False),
        "unsigned short":     Primitive(2, False),
        "unsigned reg":       Primitive(8, False),
        "syscall_result":     Primitive(8, True),
        # stdint types
        "uint8_t":            Primitive(1, False),
        "uint16_t":           Primitive(2, False),
        "uint32_t":           Primitive(4, False),
        "uint64_t":           Primitive(8, False),
        "int8_t":             Primitive(1, True),
        "int16_t":            Primitive(2, True),
        "int32_t":            Primitive(4, True),
        "int64_t":            Primitive(8, True),
        # Wasm32 specifics — both are 4-byte unsigned wasm offsets/indices
        "ptr":                Primitive(4, False),   # all pointers are 4 bytes in wasm32
        "ptr_func":           Primitive(4, False),   # function pointer = table index
    }

    type_aliases: dict[str, str] = {
        # Syscall argument types
        "off_t":        "long long",
        "size_t":       "unsigned int",
        "nfds_t":       "unsigned long long",
        "socklen_t":    "unsigned int",
        "clockid_t":    "int",
        "mode_t":       "int",
        "uid_t":        "int",
        "pid_t":        "int",
        "gid_t":        "int",
        # Syscall param reference types
        "rlim_t":       "unsigned long long",
        "time_t":       "int64_t",
        "suseconds_t":  "int64_t",
        "sa_family_t":  "unsigned short",
        "ino_t":        "uint64_t",
        "dev_t":        "uint64_t",
        "nlink_t":      "unsigned reg",
        "blksize_t":    "long",
        "blkcnt_t":     "int64_t",
        "fsblkcnt_t":   "uint64_t",
        "fsfilcnt_t":   "uint64_t",
        "epoll_data_t": "int64_t",
    }

    array_types: dict[str, ArrayType] = {
        "sigset_t":  ArrayType("long",     16),
        "fsid_t":    ArrayType("int32_t",  2),
        "uts_str":   ArrayType("uint8_t",  65),
    }

    # Populated by StructDef.__post_init__ during module load
    struct_defs: dict[str, StructDef] = {}

    @dataclass
    class StructDef:
        """A struct type with explicit fields and computed byte-level layout."""
        name: str
        fields: list[StructField]
        expected_size: int | None = None
        packed: bool = False   # __attribute__((packed)) — no field padding, alignment=1

        # Computed in __post_init__
        field_offsets: list[int] = field(default_factory=list, init=False, repr=False)
        field_sizes: list[int] = field(default_factory=list, init=False, repr=False)
        total_size: int = field(default=0, init=False)
        alignment: int = field(default=1, init=False)

        def __post_init__(self):
            self.field_offsets, self.field_sizes, self.total_size, self.alignment = \
                TypeRegistry.compute_struct_layout(self.fields, packed=self.packed)
            if self.expected_size is not None:
                assert self.total_size == self.expected_size, \
                    f"{self.name}: computed size {self.total_size} != expected {self.expected_size}"
            assert self.name not in TypeRegistry.struct_defs, \
                f"Duplicate struct definition: '{self.name}'"
            TypeRegistry.struct_defs[self.name] = self

    # --- Resolution (classmethods — usable before any instance exists) ---

    @classmethod
    def parse_inline_array(cls, type_name: str) -> tuple[int, str] | None:
        """Parse 'T[N]' inline array string. Returns (count, element_type) or None."""
        m = cls._INLINE_ARRAY_RE.match(type_name)
        if m:
            return (int(m.group(2)), m.group(1))
        return None

    @classmethod
    def resolve_primitive(cls, type_name: str) -> Primitive:
        """Walk full alias chain to leaf Primitive.
        E.g., 'off_t' -> Primitive(8, True), 'unsigned int' -> Primitive(4, False).
        """
        if type_name in cls.c_primitives:
            return cls.c_primitives[type_name]
        if type_name in cls.type_aliases:
            return cls.resolve_primitive(cls.type_aliases[type_name])
        raise RuntimeError(f"Cannot resolve '{type_name}' to a primitive "
                           f"(not in c_primitives or type_aliases)")

    @classmethod
    def resolve_size(cls, type_name: str) -> int:
        """Return the byte size of any type (primitive, alias, array, struct, inline array, pointer)."""
        if type_name.endswith('*'):
            return cls.c_primitives['ptr'].size   # wasm32 pointer
        if type_name in cls.c_primitives:
            return cls.c_primitives[type_name].size
        if type_name in cls.type_aliases:
            return cls.resolve_size(cls.type_aliases[type_name])
        if type_name in cls.array_types:
            at = cls.array_types[type_name]
            return at.count * cls.resolve_size(at.element_type)
        if type_name in cls.struct_defs:
            return cls.struct_defs[type_name].total_size
        arr = cls.parse_inline_array(type_name)
        if arr:
            count, elem = arr
            return count * cls.resolve_size(elem)
        raise RuntimeError(f"Cannot resolve size for type '{type_name}'")

    @classmethod
    def resolve_alignment(cls, type_name: str) -> int:
        """Return the alignment requirement (bytes) for any type."""
        if type_name.endswith('*'):
            return cls.c_primitives['ptr'].size   # wasm32 pointer alignment
        if type_name in cls.c_primitives:
            return cls.c_primitives[type_name].size  # natural alignment
        if type_name in cls.type_aliases:
            return cls.resolve_alignment(cls.type_aliases[type_name])
        if type_name in cls.array_types:
            return cls.resolve_alignment(cls.array_types[type_name].element_type)
        if type_name in cls.struct_defs:
            return cls.struct_defs[type_name].alignment
        arr = cls.parse_inline_array(type_name)
        if arr:
            _, elem = arr
            return cls.resolve_alignment(elem)
        raise RuntimeError(f"Cannot resolve alignment for type '{type_name}'")

    @classmethod
    def compute_struct_layout(cls, fields: list[StructField], packed: bool = False) -> tuple[list[int], list[int], int, int]:
        """Compute struct layout using C alignment rules.

        Returns (field_offsets, field_sizes, total_size, struct_alignment).
        StructFields with explicit offsets override the computed position.
        If packed=True, all fields are placed with alignment 1 (no padding).
        """
        offsets: list[int] = []
        sizes: list[int] = []
        current_offset = 0
        max_alignment = 1

        for f in fields:
            f_size = cls.resolve_size(f.type_name)
            f_align = 1 if packed else cls.resolve_alignment(f.type_name)
            max_alignment = max(max_alignment, f_align)

            if f.offset is not None:
                assert f.offset >= current_offset, \
                    f"StructField '{f.name}': explicit offset {f.offset} overlaps " \
                    f"previous field end at {current_offset}"
                current_offset = f.offset
            else:
                current_offset = (current_offset + f_align - 1) & ~(f_align - 1)

            offsets.append(current_offset)
            sizes.append(f_size)
            current_offset += f_size

        total_size = (current_offset + max_alignment - 1) & ~(max_alignment - 1)
        return offsets, sizes, total_size, max_alignment

    @classmethod
    def is_primitive(cls, type_name: str) -> bool:
        """Check if type_name is a C primitive."""
        return type_name in cls.c_primitives

    @classmethod
    def reduce_alias(cls, type_name: str) -> str:
        """Replace a type alias with its immediate base type.
        E.g., 'off_t' -> 'long long', 'mode_t' -> 'int', 'time_t' -> 'int64_t'.
        """
        return cls.type_aliases[type_name]

    # --- Validation ---

    @classmethod
    def validate(cls):
        """Validate all type references resolve."""
        errors = []

        for name, target in cls.type_aliases.items():
            try:
                cls.resolve_size(target)
            except RuntimeError as e:
                errors.append(f"type_alias '{name}' -> '{target}': {e}")

        for name, at in cls.array_types.items():
            try:
                cls.resolve_size(at.element_type)
            except RuntimeError as e:
                errors.append(f"array_type '{name}' element '{at.element_type}': {e}")

        for sd in cls.struct_defs.values():
            for f in sd.fields:
                try:
                    cls.resolve_size(f.type_name)
                except RuntimeError as e:
                    errors.append(f"struct '{sd.name}' field '{f.name}' type '{f.type_name}': {e}")

        if errors:
            for e in errors:
                logging.error(e)
            raise RuntimeError(f"{len(errors)} type resolution errors found")

        logging.info("All type references validated successfully.")

    @classmethod
    def print_struct_layouts(cls):
        """Print computed struct layouts for verification."""
        for sd in cls.struct_defs.values():
            print(f"\n{sd.name} (size={sd.total_size}, align={sd.alignment}):")
            for f, offset, size in zip(sd.fields, sd.field_offsets, sd.field_sizes):
                marker = " [explicit]" if f.offset is not None else ""
                print(f"  +{offset:>4}  {f.name:<24} {f.type_name:<28} ({size} bytes){marker}")
            print(f"  total: {sd.total_size} bytes")


# =============================================================================
# Struct Definitions
# ORDER MATTERS: referenced structs must be defined before their dependents.
# =============================================================================

# --- Leaf structs (no struct field dependencies) ---

TypeRegistry.StructDef("struct timespec", [
    StructField("tv_sec",  "time_t"),
    StructField("tv_nsec", "long"),
], expected_size=16)

TypeRegistry.StructDef("struct timeval", [
    StructField("tv_sec",  "time_t"),
    StructField("tv_usec", "suseconds_t"),
], expected_size=16)

TypeRegistry.StructDef("struct timezone", [
    StructField("tz_minuteswest", "int"),
    StructField("tz_dsttime",     "int"),
], expected_size=8)

TypeRegistry.StructDef("stack_t", [
    StructField("ss_sp",    "void*"),
    StructField("ss_flags", "int32_t"),
    StructField("ss_size",  "size_t"),
])

TypeRegistry.StructDef("struct dirent", [
    StructField("d_ino",    "ino_t"),
    StructField("d_off",    "off_t"),
    StructField("d_reclen", "unsigned short"),
    StructField("d_type",   "uint8_t"),
    StructField("d_name",   "uint8_t[256]"),
])

TypeRegistry.StructDef("struct epoll_event", [
    StructField("events", "uint32_t"),
    StructField("data",   "epoll_data_t"),
], packed=True, expected_size=12)

TypeRegistry.StructDef("struct iovec", [
    StructField("iov_base", "void*"),
    StructField("iov_len",  "size_t"),
])

TypeRegistry.StructDef("struct pollfd", [
    StructField("fd",      "int32_t"),
    StructField("events",  "short"),
    StructField("revents", "short"),
])

TypeRegistry.StructDef("struct rlimit", [
    StructField("rlim_cur", "rlim_t"),
    StructField("rlim_max", "rlim_t"),
], expected_size=16)

TypeRegistry.StructDef("struct sigaction", [
    StructField("sa_handler",  "ptr_func"),
    StructField("sa_mask",     "sigset_t"),
    StructField("sa_flags",    "int32_t"),
    StructField("sa_restorer", "ptr_func"),
])

TypeRegistry.StructDef("struct sockaddr", [
    StructField("sa_family", "sa_family_t"),
    StructField("sa_data",   "uint8_t[14]"),
])

TypeRegistry.StructDef("struct statfs", [
    StructField("f_type",    "unsigned long"),
    StructField("f_bsize",   "unsigned long"),
    StructField("f_blocks",  "fsblkcnt_t"),
    StructField("f_bfree",   "fsblkcnt_t"),
    StructField("f_bavail",  "fsblkcnt_t"),
    StructField("f_files",   "fsfilcnt_t"),
    StructField("f_ffree",   "fsfilcnt_t"),
    StructField("f_fsid",    "fsid_t"),
    StructField("f_namelen", "unsigned long"),
    StructField("f_frsize",  "unsigned long"),
    StructField("f_flags",   "unsigned long"),
    StructField("f_spare",   "unsigned long[4]"),
])

TypeRegistry.StructDef("stx_time", [
    StructField("tv_sec", "int64_t"),
    StructField("tv_nsec", "uint32_t"),
    StructField("pad",     "int32_t"),
], expected_size=16)

TypeRegistry.StructDef("struct sysinfo", [
    StructField("uptime",    "unsigned long"),
    StructField("loads",     "unsigned long[3]"),
    StructField("totalram",  "unsigned long"),
    StructField("freeram",   "unsigned long"),
    StructField("sharedram", "unsigned long"),
    StructField("bufferram", "unsigned long"),
    StructField("totalswap", "unsigned long"),
    StructField("freeswap",  "unsigned long"),
    StructField("procs",     "unsigned short"),
    StructField("pad",       "unsigned short"),
    StructField("totalhigh", "unsigned long"),
    StructField("freehigh",  "unsigned long"),
    StructField("mem_unit",  "uint32_t"),
    StructField("reserved",  "uint8_t[256]"),
])

TypeRegistry.StructDef("struct utimbuf", [
    StructField("actime",  "time_t"),
    StructField("modtime", "time_t"),
], expected_size=16)

TypeRegistry.StructDef("struct utsname", [
    StructField("sysname",    "uts_str"),
    StructField("nodename",   "uts_str"),
    StructField("release",    "uts_str"),
    StructField("version",    "uts_str"),
    StructField("machine",    "uts_str"),
    StructField("domainname", "uts_str"),
])

TypeRegistry.StructDef("fd_set", [
    StructField("fds_bits", "uint8_t[128]"),
])

TypeRegistry.StructDef("cpu_set_t", [
    StructField("bits", "uint8_t[128]"),
])

# --- Structs with struct field dependencies ---

TypeRegistry.StructDef("struct itimerval", [
    StructField("it_interval", "struct timeval"),
    StructField("it_value",    "struct timeval"),
])

TypeRegistry.StructDef("struct msghdr", [
    StructField("msg_name",       "void*"),
    StructField("msg_namelen",    "socklen_t"),
    StructField("msg_iov",        "struct iovec*"),
    StructField("msg_iovlen",     "int32_t"),
    StructField("pad1",           "int32_t"),
    StructField("msg_control",    "void*"),
    StructField("msg_controllen", "socklen_t"),
    StructField("pad2",           "int32_t"),
    StructField("msg_flags",      "int32_t"),
])

TypeRegistry.StructDef("struct rusage", [
    StructField("ru_utime",    "struct timeval"),
    StructField("ru_stime",    "struct timeval"),
    StructField("ru_maxrss",   "long"),
    StructField("ru_ixrss",    "long"),
    StructField("ru_idrss",    "long"),
    StructField("ru_isrss",    "long"),
    StructField("ru_minflt",   "long"),
    StructField("ru_majflt",   "long"),
    StructField("ru_nswap",    "long"),
    StructField("ru_inblock",  "long"),
    StructField("ru_oublock",  "long"),
    StructField("ru_msgsng",   "long"),
    StructField("ru_msgrcv",   "long"),
    StructField("ru_nsignals", "long"),
    StructField("ru_nvcsw",    "long"),
    StructField("ru_nivcsw",   "long"),
    StructField("reserved",    "long[16]"),
])

TypeRegistry.StructDef("struct stat", [
    StructField("st_dev",     "dev_t"),
    StructField("st_ino",     "ino_t"),
    StructField("st_nlink",   "nlink_t"),
    StructField("st_mode",    "mode_t"),
    StructField("st_uid",     "uid_t"),
    StructField("st_gid",     "gid_t"),
    StructField("pad0",       "uint32_t"),
    StructField("st_rdev",    "dev_t"),
    StructField("st_size",    "off_t"),
    StructField("st_blksize", "blksize_t"),
    StructField("st_blocks",  "blkcnt_t"),
    StructField("st_atim",    "struct timespec"),
    StructField("st_mtim",    "struct timespec"),
    StructField("st_ctim",    "struct timespec"),
    StructField("unused",     "long[3]"),
], expected_size=144)

TypeRegistry.StructDef("struct statx", [
    StructField("stx_mask",            "uint32_t"),
    StructField("stx_blksize",         "uint32_t"),
    StructField("stx_attributes",      "uint64_t"),
    StructField("stx_nlink",           "uint32_t"),
    StructField("stx_uid",             "uint32_t"),
    StructField("stx_gid",             "uint32_t"),
    StructField("stx_mode",            "uint16_t"),
    StructField("pad1",                "uint16_t"),
    StructField("stx_ino",             "uint64_t"),
    StructField("stx_size",            "uint64_t"),
    StructField("stx_blocks",          "uint64_t"),
    StructField("stx_attributes_mask", "uint64_t"),
    StructField("stx_atime",           "stx_time"),
    StructField("stx_btime",           "stx_time"),
    StructField("stx_ctime",           "stx_time"),
    StructField("stx_mtime",           "stx_time"),
    StructField("stx_rdev_major",      "uint32_t"),
    StructField("stx_rdev_minor",      "uint32_t"),
    StructField("stx_dev_major",       "uint32_t"),
    StructField("stx_dev_minor",       "uint32_t"),
    StructField("spare",               "uint64_t[14]"),
])


if __name__ == '__main__':
    import sys
    logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')

    try:
        TypeRegistry.validate()
    except RuntimeError:
        sys.exit(1)

    TypeRegistry.print_struct_layouts()

    print(f"\nTypeRegistry: {len(TypeRegistry.c_primitives)} primitives, "
          f"{len(TypeRegistry.type_aliases)} aliases, {len(TypeRegistry.array_types)} array types, "
          f"{len(TypeRegistry.struct_defs)} structs")

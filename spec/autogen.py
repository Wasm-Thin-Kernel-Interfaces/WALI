import re
import argparse
import logging
import textwrap
from pathlib import Path
from dataclasses import dataclass, fields
from typing import List, Dict, Any, Callable, Set, Optional, Tuple, Type

# Import definition objects
from syscall_definitions import SYSCALLS, AUX_SYSCALLS, Syscall, AuxSyscall, Nrs, ScArg
from types_abi import TypeSystem

@dataclass
class GeneratorContext:
    archs: List[str]
    syscalls: Dict[str, Syscall]
    aux_syscalls: Dict[str, AuxSyscall]

    @classmethod
    def create(cls) -> 'GeneratorContext':
        logging.info("Creating context...")
        archs = [f.name for f in fields(Nrs)]
        return cls(archs, SYSCALLS, AUX_SYSCALLS)


# --- Base Generator ---
class StubGenerator:
    # Shared paths for all generators
    SCRIPT_DIR = Path(__file__).parent
    TEMPLATES_DIR = SCRIPT_DIR / 'templates'
    TS = TypeSystem

    def __init__(self, spath: Path, context: GeneratorContext):
        self.spath = spath
        self.spath.mkdir(parents=True, exist_ok=True)
        self.ctx = context

    @property
    def archs(self) -> List[str]:
        return self.ctx.archs

    @property
    def syscalls(self) -> Dict[str, Syscall]:
        return self.ctx.syscalls

    @property
    def aux_syscalls(self) -> Dict[str, AuxSyscall]:
        return self.ctx.aux_syscalls

    def generate(self):
        raise NotImplementedError

    def write_file(self, filename: str, content: str):
        """Writes content to file at self.spath / filename."""
        filepath = self.spath / filename
        filepath.parent.mkdir(parents=True, exist_ok=True)
        with open(filepath, 'w') as f:
            f.write(content)

    def gen_and_write(self, stub_fn: Callable, filename: str, include_unimplemented_calls: bool = False, prelude: str = "", postlude: str = ""):
        # Call stubs for each syscall
        buf = [stub_fn(sc) for sc in self.syscalls.values() if include_unimplemented_calls or sc.implemented]
        # Construct content: prelude + joined stubs + postlude
        content_parts = []
        if prelude: content_parts.append(prelude)
        content_parts.append('\n'.join(buf))
        if postlude: content_parts.append(postlude)
        self.write_file(filename, '\n'.join(content_parts))


# --- Specific Generators ---

class LibcGenerator(StubGenerator):
    def generate(self):
        self._gen_c_stubs()
        self._gen_rust_stubs()

    def _ptr_anonymize(self, args: List[ScArg]) -> List[str]:
        """Anonymize pointer argument types to void* for non-basic pointer types."""
        def f(arg: ScArg):
            (indir, base) = arg.ptr_split(max=1)
            return 'void*' if indir > 0 and not self.TS.is_primitive(base) else arg
        
        return [f(arg) for arg in args]

    def rustify_args(self, sc: Syscall) -> List[str]:
        """Convert argument types to Rust compatible types"""
        ptr_wit = self.TS.c_primitives['ptr'].wit_name
        def f(arg: ScArg):
            prim = ptr_wit if arg.is_ptr() or arg.is_fn_ptr() else self.TS.resolve_primitive(arg).wit_name
            return prim if not prim.startswith('s') else 'i' + prim[1:]

        return [f(x) for x in sc.args_reduce()]


    def _gen_c_stubs(self):
        def def_stub(sc: Syscall):
            arglist = ','.join(self._ptr_anonymize(sc.args_reduce()))
            s = f"WALI_SYSCALL_DEF ({sc.name}, {arglist});"
            for alias in sc.aliases:
                s += f"\n#define __syscall_SYS_{alias} __syscall_SYS_{sc.name}"
            return s

        def case_stub(sc: Syscall):
            anon_args = self._ptr_anonymize(sc.args_reduce())
            arglist = ','.join([f"({typ})a{i+1}" for i, typ in enumerate(anon_args)])
            return f"\t\tCASE_SYSCALL ({sc.name}, {arglist});"

        def case_var_stub(sc: Syscall):
            anon_args = self._ptr_anonymize(sc.args_reduce())
            arglist = ','.join([f"va_arg(ap, {typ})" for typ in anon_args])
            return f"\t\tCASE_SYSCALL ({sc.name}, {arglist});"

        self.gen_and_write(def_stub, 'defs.out', True)
        self.gen_and_write(case_stub, 'case.out')
        self.gen_and_write(case_var_stub, 'case_var.out')

    def _gen_rust_stubs(self):
        rpath = self.spath / 'rust'
        rpath.mkdir(parents=True, exist_ok=True)


        def rust_def_stub(sc: Syscall):
            rust_args = self.rustify_args(sc)
            argtys = ', '.join([f"a{i+1}: {typ}" for i, typ in enumerate(rust_args)])
            
            code = textwrap.dedent(f"""\
                /* {sc.nr} */
                #[link_name = "SYS_{sc.name}"]
                pub fn __syscall_SYS_{sc.name}({argtys}) -> ::c_long;""")
            return textwrap.indent(code, '\t')

        def rust_match_stub(sc: Syscall):
            if sc.implemented:
                call_args = ', '.join([f"a{x}" for x, _ in enumerate(sc.args_reduce())])
                code = f"super::SYS_{sc.name} => syscall_match_arm!(SYS_{sc.name}, args, {call_args}),"
            else:
                code = f"super::SYS_{sc.name} => unimplemented!(\"WALI syscall '{sc.name}' ({sc.nr}) unimplemented!\"),"
            return textwrap.indent(code, '\t\t')

        def_prelude = textwrap.dedent("""\
            // --- Autogenerated from WALI/scripts/autogen.py ---
            #[link(wasm_import_module = "wali")]
            extern "C" {""")
        def_postlude = "\n}"
        
        # We need a slightly custom writer for rust subdirectory or override gen_and_write
        # But base class gen_and_write writes to self.spath / filename.
        # We can pass 'rust/defs.out' as filename.
        self.gen_and_write(rust_def_stub, 'rust/defs.out', False, def_prelude, def_postlude)

        match_prelude = textwrap.dedent("""\
            #[no_mangle]
            pub unsafe extern "C" fn syscall(num: ::c_long, mut args: ...) -> ::c_long {
                use core::unimplemented;
                match num {""")
        match_postlude = textwrap.dedent("""\
                _ => unimplemented!("WALI syscall number {} out-of-scope!", num),
            }
        }""")
        self.gen_and_write(rust_match_stub, 'rust/match.out', True, match_prelude, match_postlude)


class WamrGenerator(StubGenerator):
    def _arg_ctype(self, arg: ScArg) -> str:
        """Map a syscall arg to its wasm-side C type.
        Memory pointers use WasmMemAddr; function pointers use WasmTableInternalIdx;
        others use a fixed-width intN_t/uintN_t."""
        if arg.is_fn_ptr():
            return "WasmTableInternalIdx"
        if arg.is_ptr():
            return "WasmMemAddr"
        prim = self.TS.resolve_primitive(arg)
        return f"{'int' if prim.signed else 'uint'}{prim.size * 8}_t"

    def _aux_fn_name(self, aux: AuxSyscall) -> str:
        """wali_<name> with any leading underscores stripped (e.g. __init -> wali_init)."""
        return "wali_" + aux.name.lstrip('_')

    def _aux_ret_type(self, aux: AuxSyscall) -> str:
        return aux.result if aux.result else "void"

    def generate(self):
        def declr_stub(sc: Syscall):
            arglist = ''.join([f", {self._arg_ctype(a)} {name}" for a, name in zip(sc.args_reduce(), sc.args_id)])
            return f"long wali_syscall_{sc.name} (wasm_exec_env_t exec_env{arglist});"

        def impl_stub(sc: Syscall):
            args_red = sc.args_reduce()
            arglist_def = ''.join([f", {self._arg_ctype(a)} {name}" for a, name in zip(args_red, sc.args_id)])

            # Construct return call args
            ret_args = []
            for argty, name in zip(args_red, sc.args_id):
                if argty.endswith('*'):
                    ret_args.append(f", MADDR({name})")
                else:
                    ret_args.append(f", {name}")
            ret_arglist = ''.join(ret_args)

            return textwrap.dedent(f"""\
                // {sc.nr} TODO
                long wali_syscall_{sc.name} (wasm_exec_env_t exec_env{arglist_def}) {{
                    SC({sc.nr}, {sc.name});
                    ERRSC({sc.name});
                    RETURN(__syscall{len(args_red)}(SYS_{sc.name}{ret_arglist}));
                }}
                """)

        def symbols_stub(sc: Syscall):
            def gen_native_args(args_list):
                params = ''.join(["I" if x.endswith("long long") or x.endswith("long") else "i" for x in args_list])
                return f"\"({params})I\""

            return "\tNSYMBOL ( {: >20}, {: >30}, {: >12} ),".format(
                "SYS_" + sc.name,
                "wali_syscall_" + sc.name,
                gen_native_args(sc.args_reduce())
            )

        def _native_sig_char(arg: ScArg) -> str:
            if arg.is_ptr() or arg.is_fn_ptr():
                return 'i'
            return 'I' if self.TS.resolve_primitive(arg).size == 8 else 'i'

        def aux_declr_stub(aux: AuxSyscall):
            arglist = ''.join([f", {self._arg_ctype(a)} {name}" for a, name in zip(aux.args, aux.args_id)])
            return f"{self._aux_ret_type(aux)} {self._aux_fn_name(aux)} (wasm_exec_env_t exec_env{arglist});"

        def aux_impl_stub(aux: AuxSyscall):
            arglist_def = ''.join([f", {self._arg_ctype(a)} {name}" for a, name in zip(aux.args, aux.args_id)])
            ret_ty = self._aux_ret_type(aux)
            tag = aux.name.lstrip('_')

            lines = [
                f"// {aux.name} TODO",
                f"{ret_ty} {self._aux_fn_name(aux)} (wasm_exec_env_t exec_env{arglist_def}) {{",
                f"    PC({tag});",
                f"    ERRSC({tag}, \"Not yet implemented\");",
            ]
            if ret_ty != "void":
                lines.append("    return 0;")
            lines.append("}\n")
            return "\n".join(lines)

        def aux_symbols_stub(aux: AuxSyscall):
            params = ''.join(_native_sig_char(a) for a in aux.args)
            ret = '' if aux.result is None else _native_sig_char(ScArg(aux.result))
            sig = f"\"({params}){ret}\""
            return "\tNSYMBOL ( {: >20}, {: >30}, {: >12} ),".format(
                aux.name, self._aux_fn_name(aux), sig
            )

        aux_declrs = '\n'.join(aux_declr_stub(a) for a in self.aux_syscalls.values())
        aux_impls = '\n'.join(aux_impl_stub(a) for a in self.aux_syscalls.values())
        aux_symbols = '\n'.join(aux_symbols_stub(a) for a in self.aux_syscalls.values())

        typedefs = "typedef uint32_t WasmMemAddr;\ntypedef uint32_t WasmTableInternalIdx;\n"
        self.gen_and_write(declr_stub, 'declr.out',
                           prelude=typedefs + "\n/* Syscalls */",
                           postlude="\n/* Auxiliary calls */\n" + aux_declrs)
        self.gen_and_write(impl_stub, 'impl.out',
                           prelude="/* Syscalls */",
                           postlude="/* Auxiliary calls */\n" + aux_impls)
        self.gen_and_write(symbols_stub, 'symbols.out',
                           prelude="\t/* Syscalls */",
                           postlude="\n\t/* Auxiliary calls */\n" + aux_symbols)


class WitGenerator(StubGenerator):
    WIT_KEYWORDS_RESERVED_MAP = {
        "flags": "flags-id",
        "type": "type-id",
        "resource": "resource-id",
        "list": "list-id",
        "option": "option-id",
    }

    # --- WIT naming helpers ---

    @staticmethod
    def _to_wit(name: str) -> str:
        """Convert type name to WIT hyphen-style (handles both spaces and underscores)."""
        return name.replace(' ', '-').replace('_', '-')

    @staticmethod
    def _is_stdint(name: str) -> bool:
        """Check if a C_PRIMITIVES key is a stdint type (uint32_t, int64_t, etc.)."""
        return name.endswith('_t') and len(name) > 1 and name[0] in 'iu'

    def _field_type_to_wit(self, type_name: str) -> str:
        """Convert a types_abi field type name to its WIT representation."""
        TS = self.TS
        # Pointer types: "struct iovec*" -> "ptr-struct-iovec", "void*" -> "ptr-void"
        if type_name.endswith('*'):
            stripped = type_name.rstrip('*')
            indirection = len(type_name) - len(stripped)
            pointee = stripped.strip()
            return ('ptr-' * indirection) + self._to_wit(pointee)
        if type_name in TS.type_aliases:
            return self._to_wit(type_name)
        if type_name in TS.array_types:
            return self._to_wit(type_name)
        if type_name in TS.struct_defs:
            return self._to_wit(type_name)
        if type_name in TS.c_primitives:
            if self._is_stdint(type_name):
                return TS.c_primitives[type_name].wit_name
            return self._to_wit(type_name)
        arr = TS.parse_inline_array(type_name)
        if arr:
            count, elem = arr
            wit_elem = self._field_type_to_wit(elem)
            return f"Array[{count}, {wit_elem}]"
        raise RuntimeError(f"Cannot convert type '{type_name}' to WIT name")

    def _resolve_arg_wit(self, arg: str) -> str:
        """Resolve a WIT-formatted syscall arg type to its WIT primitive if it's a basic type."""
        # arg is already in WIT format (hyphens). Check if it's a basic C type alias.
        if arg in self._basic_types_wit:
            return self._basic_types_wit[arg]
        return arg

    # --- Generation ---

    def generate(self):
        # Build a WIT-format lookup for basic C types: "unsigned-int" -> "u32", etc.
        self._basic_types_wit = {}
        for c_name, prim in self.TS.c_primitives.items():
            if not self._is_stdint(c_name):
                self._basic_types_wit[self._to_wit(c_name)] = prim.wit_name

        buf = []
        buf_aux = []
        uniq_ptr_types = set()
        aux_ptr_types = set()

        # Collect pointer types referenced in struct fields (so their ptr-X
        # aliases get emitted alongside those from syscall args).
        for sd in self.TS.struct_defs.values():
            for f in sd.fields:
                if f.type_name.endswith('*'):
                    uniq_ptr_types.add(self._field_type_to_wit(f.type_name))

        def transform_ptr_arg(arg: ScArg):
            if arg.is_fn_ptr():
                return 'ptr-func'
            arg_no_ptr = arg.rstrip('*')
            ptr_indirection = len(arg) - len(arg_no_ptr)
            return ("ptr-" * ptr_indirection) + arg_no_ptr

        for sc in self.syscalls.values():
            args = [ScArg(x.strip().replace(' ', '-').replace('_', '-')) for x in sc.args]
            args = [transform_ptr_arg(x) for x in args]

            up_types = set([x for x in args if x.startswith('ptr-')])
            uniq_ptr_types.update(up_types)

            if sc.implemented:
                l1 = f"\t// [{sc.nr}] {sc.name}({', '.join(sc.args)})"
                wit_args = ', '.join([
                    f"{self.WIT_KEYWORDS_RESERVED_MAP.get(id, id.replace('_', '-'))}: {self._resolve_arg_wit(arg)}"
                    for (id, arg) in zip(sc.args_id, args)
                ])
                l2 = f"\tSYS-{sc.name.replace('_', '-')}: func({wit_args}) -> syscall-result;"
                buf.append(l1 + '\n' + l2)

        for sc in self.aux_syscalls.values():
            args = [x.strip().replace(' ', '-').replace('_', '-') for x in sc.args]
            args = [transform_ptr_arg(ScArg(x)) for x in args]

            up_types = set([x for x in args if x.startswith('ptr-')])
            aux_ptr_types.update(up_types)

            l1 = f"\t// {sc.name}({', '.join(sc.args)})"
            wit_args = ', '.join([
                f"{self.WIT_KEYWORDS_RESERVED_MAP.get(id, id.replace('_', '-'))}: {self._resolve_arg_wit(arg)}"
                for (id, arg) in zip(sc.args_id, args)
            ])

            name = sc.name.lstrip('_').replace('_', '-')
            start_sig = f"\t{name}: func({wit_args})"

            res_ty = None
            if sc.result:
                res_ty = self._resolve_arg_wit(sc.result)

            if res_ty:
                l2 = f"{start_sig} -> {res_ty};"
            else:
                l2 = f"{start_sig};"

            buf_aux.append(l1 + '\n' + l2)

        buf = list(filter(bool, buf))
        buf_aux = list(filter(bool, buf_aux))

        self._generate_wit_file(buf, buf_aux, uniq_ptr_types, aux_ptr_types)

    def _build_wit_comp_types(self) -> dict:
        """Build the WIT type alias declarations from the type system."""
        comp_types = {}

        # C primitive type aliases (e.g., "type long = s64;")
        for c_name, prim in self.TS.c_primitives.items():
            if self._is_stdint(c_name):
                continue
            comp_types[self._to_wit(c_name)] = prim.wit_name

        # Type aliases (e.g., "type off-t = long-long;")
        for alias_name, target in self.TS.type_aliases.items():
            wit_alias = self._to_wit(alias_name)
            if target in self.TS.c_primitives:
                if self._is_stdint(target):
                    comp_types[wit_alias] = self.TS.c_primitives[target].wit_name
                else:
                    comp_types[wit_alias] = self._to_wit(target)
            else:
                comp_types[wit_alias] = self._to_wit(target)

        # Array types (e.g., "type sigset-t = Array[128, u8];")
        for arr_name, arr_type in self.TS.array_types.items():
            wit_name = self._to_wit(arr_name)
            elem_wit = self.TS.c_primitives[arr_type.element_type].wit_name
            comp_types[wit_name] = f"Array[{arr_type.count}, {elem_wit}]"

        return comp_types

    def _generate_record_content(self) -> str:
        """Generate WIT record definitions from struct defs."""
        lines = []
        for sd in self.TS.struct_defs.values():
            wit_name = self._to_wit(sd.name)
            lines.append(f"record {wit_name} {{")
            for i, f in enumerate(sd.fields):
                wit_field_name = self._to_wit(f.name)
                wit_type = self._field_type_to_wit(f.type_name)
                comma = "," if i < len(sd.fields) - 1 else ""
                lines.append(f"  {wit_field_name}: {wit_type}{comma}")
            lines.append("}")
            lines.append("")
        return '\n'.join(lines)

    def _generate_wit_file(self, buf, buf_aux, uniq_ptr_types, aux_ptr_types):
        comp_types = self._build_wit_comp_types()

        type_if = ["interface types {"] + \
                  [f"\ttype {k} = {v};" for k, v in comp_types.items()] + \
                  ["}"]

        # Generate records from struct defs
        record_content = self._generate_record_content()

        # Validate that all pointer types in syscalls have matching records
        record_types = set(self._to_wit(name) for name in self.TS.struct_defs.keys())
        sc_ptr_types = set()

        for x in uniq_ptr_types:
            x_no_ptr = x
            while x_no_ptr.startswith("ptr-"):
                x_no_ptr = x_no_ptr.replace("ptr-", "", 1)

            is_skippable = (x_no_ptr in self._basic_types_wit or
                            x_no_ptr in comp_types or
                            x_no_ptr in ['void', 'char'])
            if not is_skippable:
                sc_ptr_types.add(x_no_ptr)

        missing_types = sc_ptr_types.difference(record_types)
        if missing_types:
            logging.warning(f"Missing Records for Complex Types: {missing_types}")
        else:
            logging.info("Successfully bound all records")

        sc_prelude = (
            ["interface syscalls {"] +
            [f"\tuse types.{{{', '.join(comp_types)}}};"] +
            ["\t/// Readable pointer types"] +
            [f"\ttype {x} = ptr;" for x in sorted(uniq_ptr_types)] +
            ["", "\t/// Record types"] +
            ["\t" + '\n\t'.join(record_content.split('\n'))] +
            [""]
        )

        aux_prelude = (
            ["interface aux {"] +
            [f"\tuse types.{{{', '.join(comp_types)}}};"] +
            [f"\tuse syscalls.{{{', '.join([x for x in aux_ptr_types if x in uniq_ptr_types])}}};"] +
            [""]
        )

        sc_if = sc_prelude + ["\t/// Syscall methods"] + buf + ["}"]
        aux_if = aux_prelude + ["\t/// Aux methods"] + buf_aux + ["}"]

        with open(self.TEMPLATES_DIR / 'wit' / 'wali.wit.template', 'r') as f:
            template = f.read()

        fill_temp = template.replace(
            '[[TYPES_STUB]]', '\n'.join(type_if)
        ).replace(
            '[[SYSCALLS_STUB]]', '\n'.join(sc_if)
        ).replace(
            '[[AUX_SYSCALLS_STUB]]', '\n'.join(aux_if)
        )

        def matchrep(matchobj):
            num_elem, ty = matchobj.groups()
            return "tuple<{}>".format(','.join([ty] * int(num_elem)))

        # No fixed-length arrays in WIT — convert Array[N, T] to tuple<T, T, ...>
        fill_temp = re.sub(r'Array\[\s*(\d+),\s*(\S+)\s*\]', matchrep, fill_temp)

        self.write_file('wali.wit', fill_temp)


class DocsGenerator(StubGenerator):
    """Generates MkDocs-compatible markdown documentation for WALI syscalls."""
    
    # Docs-specific paths (uses inherited TEMPLATES_DIR)
    DOCS_TEMPLATES_DIR = StubGenerator.TEMPLATES_DIR / 'docs'
    OVERRIDES_FILE = DOCS_TEMPLATES_DIR / 'overrides' / 'syscalls.yaml'
    
    # Badge colors for architectures
    ARCH_COLORS = {
        'x86_64': '4c8cbf',   # Blue
        'ARM64': 'c73e3a',    # Red  
        'RV64': '6b4c9a',     # Purple
    }
    
    def __init__(self, spath: Path, context: GeneratorContext):
        super().__init__(spath, context)
        self.overrides = self._load_overrides()
        self.templates = self._load_templates()
    
    def _load_overrides(self) -> Dict[str, Any]:
        """Load manual documentation overrides from YAML file."""
        import yaml
        if self.OVERRIDES_FILE.exists():
            with open(self.OVERRIDES_FILE) as f:
                return yaml.safe_load(f) or {}
        return {}
    
    def _load_templates(self) -> Dict[str, str]:
        """Load markdown templates."""
        templates = {}
        for name in ['syscall', 'aux', 'specification']:
            tpl_path = self.DOCS_TEMPLATES_DIR / f'{name}.md.template'
            if not tpl_path.exists():
                raise FileNotFoundError(f"Required template not found: {tpl_path}")
            with open(tpl_path) as f:
                templates[name] = f.read()
        return templates

    def generate(self):
        """Generate all documentation files."""
        self._gen_specification_index()
        self._gen_syscall_pages()
        self._gen_aux_pages()
        self._gen_type_pages()
        self._copy_static_docs()
    
    def _copy_static_docs(self):
        """Copy static docs (index.md, assets/, stylesheets/, etc.) to output."""
        import shutil
        src_index = self.DOCS_TEMPLATES_DIR / 'index.md'
        if src_index.exists():
            shutil.copy(src_index, self.spath / 'index.md')

        for subdir in ('assets', 'stylesheets'):
            src = self.DOCS_TEMPLATES_DIR / subdir
            if src.exists():
                dst = self.spath / subdir
                if dst.exists():
                    shutil.rmtree(dst)
                shutil.copytree(src, dst)
    
    def _make_badge(self, arch: str, number: int) -> str:
        """Generate a Shields.io style badge for syscall number."""
        color = self.ARCH_COLORS.get(arch, '555555')
        if number < 0:
            # Show dash for unsupported architectures
            return f"![{arch}](https://img.shields.io/badge/{arch}----{color})"
        # Using shields.io static badge format
        return f"![{arch}](https://img.shields.io/badge/{arch}-{number}-{color})"
    
    def _make_badges(self, sc: Syscall) -> str:
        """Generate all architecture badges for a syscall."""
        badges = [
            self._make_badge('x86_64', sc.nrs.x86_64),
            self._make_badge('ARM64', sc.nrs.arm64),
            self._make_badge('RV64', sc.nrs.rv64),
        ]
        return " ".join(badges)
    
    def _c_signature(self, name: str, args: List, args_id: List, ret_type: str = "long") -> str:
        """Generate C function signature."""
        args_str = ", ".join([f"{ty} {id}" for ty, id in zip(args, args_id)]) if args else "void"
        return f"{ret_type} {name}({args_str});"
    
    def _wasm_signature(self, name: str, args: List, args_id: List, ret_type: str = "i64", is_syscall: bool = True) -> str:
        """Generate Wasm32 import signature in WAT format."""
        # Map C types to Wasm types
        ptr_wit = self.TS.c_primitives['ptr'].wit_name
        def to_wasm_type(ty: ScArg) -> str:
            prim = ptr_wit if (ty.is_ptr() or ty.is_fn_ptr()) else self.TS.resolve_primitive(ty).wit_name
            bitwidth = "32" if int(prim[1:]) < 32 else prim[1:]
            return f"i{bitwidth}"
        
        params = " ".join([f"(param ${id} {to_wasm_type(ty)})" for ty, id in zip(args, args_id)])
        result = f"(result {ret_type})" if ret_type else ""
        
        # Syscalls use SYS_ prefix, aux functions don't
        import_name = f"SYS_{name}" if is_syscall else name
        
        func_body = f"(func {params} {result})".strip()
        return f'(import "wali" "{import_name}" {func_body})'
    
    def _gen_specification_index(self):
        """Generate the specification overview page with syscall table."""
        supp_sc = [s for s in self.syscalls.values() if s.implemented]
        
        # Build syscall table - only name and arch numbers, with SYS_ prefix
        header = "| Syscall | x86_64 | ARM64 | RV64 |"
        sep = "|---------|--------|-------|------|"
        rows = []
        for s in sorted(supp_sc, key=lambda x: x.nrs.x86_64):
            x86 = s.nrs.x86_64 if s.nrs.x86_64 >= 0 else "-"
            arm = s.nrs.arm64 if s.nrs.arm64 >= 0 else "-"
            rv = s.nrs.rv64 if s.nrs.rv64 >= 0 else "-"
            rows.append(f"| [SYS_{s.name}](syscalls/{s.name}.md) | {x86} | {arm} | {rv} |")
        
        syscall_table = "\n".join([header, sep] + rows)
        
        # Build aux table - single column with just names
        aux_header = "| Function |"
        aux_sep = "|----------|"
        aux_rows = []
        for s in self.aux_syscalls.values():
            name_clean = s.name.lstrip('_')
            aux_rows.append(f"| [{s.name}](aux/{name_clean}.md) |")
        
        aux_table = "\n".join([aux_header, aux_sep] + aux_rows)
        
        content = self.templates['specification'].replace(
            '[[NUM_SYSCALLS]]', str(len(supp_sc))
        ).replace(
            '[[NUM_AUX]]', str(len(self.aux_syscalls))
        ).replace(
            '[[SYSCALL_TABLE]]', syscall_table
        ).replace(
            '[[AUX_TABLE]]', aux_table
        )
        
        self.write_file('specification/index.md', content)
    
    def _gen_syscall_pages(self):
        """Generate individual syscall documentation pages."""
        (self.spath / 'specification' / 'syscalls').mkdir(parents=True, exist_ok=True)
        
        for sc in self.syscalls.values():
            if not sc.implemented:
                continue
            self._gen_syscall_page(sc)
    
    def _gen_syscall_page(self, sc: Syscall):
        """Generate a single syscall documentation page."""
        override = self.overrides.get(sc.name, {})

        # Build signatures
        c_sig = self._c_signature(sc.name, sc.args, sc.args_id)
        wasm_sig = self._wasm_signature(sc.name, sc.args, sc.args_id)

        # Build badges
        badges = self._make_badges(sc)

        # Description from override only
        description = override.get('description', '')

        # Notes section
        notes = override.get('notes', '')
        notes_md = f"## Notes\n\n{notes}" if notes else ""

        # Referenced types section
        refs = self._syscall_referenced_types(sc)
        if refs:
            lines = ["## Referenced Types", ""]
            for type_name, arg_names in refs:
                via = ", ".join(f"`{n}`" for n in arg_names)
                lines.append(f"- {self._type_link(type_name, '../types.md')} (via {via})")
            refs_md = "\n".join(lines) + "\n"
        else:
            refs_md = ""

        content = self.templates['syscall'].replace(
            '[[NAME]]', sc.name
        ).replace(
            '[[BADGES]]', badges
        ).replace(
            '[[DESCRIPTION]]', description
        ).replace(
            '[[C_SIGNATURE]]', c_sig
        ).replace(
            '[[WASM_SIGNATURE]]', wasm_sig
        ).replace(
            '[[NOTES]]', refs_md + notes_md
        )

        self.write_file(f'specification/syscalls/{sc.name}.md', content)
    
    def _gen_aux_pages(self):
        """Generate auxiliary function documentation pages."""
        (self.spath / 'specification' / 'aux').mkdir(parents=True, exist_ok=True)
        
        for aux in self.aux_syscalls.values():
            self._gen_aux_page(aux)
    
    def _gen_aux_page(self, aux: AuxSyscall):
        """Generate a single auxiliary function documentation page."""
        override = self.overrides.get(aux.name, {})
        
        # Build signatures
        ret_type = aux.result if aux.result else "void"
        c_sig = self._c_signature(aux.name, aux.args, aux.args_id, ret_type)
        wasm_ret = "i32" if ret_type in ("int", "unsigned int") else ("i64" if ret_type else "")
        wasm_sig = self._wasm_signature(aux.name, aux.args, aux.args_id, wasm_ret, is_syscall=False)
        
        # Description from override only
        description = override.get('description', '')
        
        # Notes section
        notes = override.get('notes', '')
        notes_md = f"## Notes\n\n{notes}" if notes else ""
        
        name_clean = aux.name.lstrip('_')
        
        content = self.templates['aux'].replace(
            '[[NAME]]', aux.name
        ).replace(
            '[[DESCRIPTION]]', description
        ).replace(
            '[[C_SIGNATURE]]', c_sig
        ).replace(
            '[[WASM_SIGNATURE]]', wasm_sig
        ).replace(
            '[[RETURN_TYPE]]', ret_type
        ).replace(
            '[[NOTES]]', notes_md
        )

        self.write_file(f'specification/aux/{name_clean}.md', content)

    # --- Type page generation ---

    @staticmethod
    def _type_page_filename(type_name: str) -> str:
        """Convert a type name to a markdown filename (e.g., 'struct stat' -> 'struct_stat.md')."""
        return type_name.replace(' ', '_').replace('*', '').strip('_') + '.md'

    @classmethod
    def _is_documented_type(cls, type_name: str) -> bool:
        """Check if type_name is a struct, alias, or named array (i.e., has a dedicated page)."""
        return (type_name in TypeSystem.struct_defs
                or type_name in TypeSystem.type_aliases
                or type_name in TypeSystem.array_types)

    def _type_link(self, type_name: str, relative_link: str = 'types.md') -> str:
        """Return a markdown link to the given type's entry, or plain text if none exists.
        All documented types live on a single combined page (specification/types.md).
        """
        if not self._is_documented_type(type_name):
            return f"`{type_name}`"
        anchor = self._alias_anchor(type_name)
        return f"[{type_name}]({relative_link}#{anchor})"

    @staticmethod
    def _alias_anchor(name: str) -> str:
        """Sanitize a type name for use as a markdown anchor."""
        return name.replace(' ', '_').replace('*', '').strip('_')

    def _syscall_referenced_types(self, sc: Syscall) -> List[Tuple[str, List[str]]]:
        """Return [(type_name, [arg_names_using_it])] for types referenced in this syscall's args.
        Preserves first-occurrence order. Skips primitive types.
        """
        order: List[str] = []
        via: Dict[str, List[str]] = {}
        for arg, arg_id in zip(sc.args, sc.args_id):
            base = arg.rstrip('*').strip()
            if self._is_documented_type(base):
                if base not in via:
                    order.append(base)
                    via[base] = []
                via[base].append(arg_id)
        return [(name, via[name]) for name in order]

    def _gen_type_pages(self):
        """Generate a single combined types page covering all structs, aliases, and named arrays."""
        self._gen_types_page()

    def _field_type_display(self, type_name: str) -> str:
        """Render a struct field type for the layout table — linked if documented."""
        # Handle pointer types: "struct iovec*" -> link to "struct iovec" page + *
        if type_name.endswith('*'):
            stripped = type_name.rstrip('*').strip()
            stars = '*' * (len(type_name) - len(stripped))
            if self._is_documented_type(stripped):
                return f"{self._type_link(stripped)}{stars}"
            return f"`{type_name}`"
        # Handle inline arrays: "long[3]" stays literal
        if TypeSystem.parse_inline_array(type_name):
            return f"`{type_name}`"
        return self._type_link(type_name)

    def _gen_types_page(self):
        """Generate a single combined page with all struct layouts + an aliases table.
        Each entry has an HTML anchor for deep-linking from syscall pages and field tables."""
        lines = ["---", "title: Types and ABI", "---", "", "# Types and ABI", ""]

        # --- Structs ---
        lines.append("## Structs")
        lines.append("")
        for sd in TypeSystem.struct_defs.values():
            anchor = self._alias_anchor(sd.name)
            lines.append(f'<a id="{anchor}"></a>')
            lines.append(f"### {sd.name}")
            lines.append("")
            lines.append("| Offset | Field | Type | Size |")
            lines.append("|--------|-------|------|------|")
            for f, offset, size in zip(sd.fields, sd.field_offsets, sd.field_sizes):
                type_disp = self._field_type_display(f.type_name)
                explicit = " *(explicit)*" if f.offset is not None else ""
                lines.append(f"| {offset} | `{f.name}`{explicit} | {type_disp} | {size} |")
            # Meta line (size / alignment / packed) as small text under the table
            meta_parts = [
                f"Total Size: <b>{sd.total_size}</b> bytes",
                f"Alignment: <b>{sd.alignment}</b> bytes",
            ]
            if sd.packed:
                meta_parts.append("Packed")
            lines.append(f'<p class="struct-meta"><small>{" · ".join(meta_parts)}</small></p>')
            lines.append('<hr class="struct-divider">')
            lines.append("")

        # --- Type aliases + named arrays (single table) ---
        lines.append("## Type Aliases")
        lines.append("")
        lines.append("| Name | Target |")
        lines.append("|------|--------|")
        for alias_name, target in TypeSystem.type_aliases.items():
            anchor = self._alias_anchor(alias_name)
            lines.append(f'| <a id="{anchor}"></a>`{alias_name}` | `{target}` |')
        for arr_name, at in TypeSystem.array_types.items():
            anchor = self._alias_anchor(arr_name)
            lines.append(f'| <a id="{anchor}"></a>`{arr_name}` | `{at.element_type}[{at.count}]` |')
        lines.append("")

        # --- C primitives ---
        lines.append("## C Primitives")
        lines.append("")
        lines.append("| Name | Size | Alignment |")
        lines.append("|------|------|-----------|")
        for c_name, prim in TypeSystem.c_primitives.items():
            anchor = self._alias_anchor(c_name)
            lines.append(f'| <a id="{anchor}"></a>`{c_name}` | {prim.size} | {prim.size} |')
        lines.append("")

        self.write_file('specification/types.md', "\n".join(lines))

# --- Registry & Main ---

GENERATORS = {
    'libc': LibcGenerator,
    'wamr': WamrGenerator,
    'wit': WitGenerator,
    'docs': DocsGenerator
}

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog='wali-autogen', 
        description="Generate WALI descriptions/implementations/stubs"
    )
    parser.add_argument('--verbose', '-v', 
                        help='Logging verbosity', 
                        choices=range(6), type=int, default=4)
    parser.add_argument('stubs', 
                        nargs='*', 
                        choices=list(GENERATORS.keys())+['all'], 
                        default='all')
    p = parser.parse_args()
    
    if p.stubs == 'all' or (isinstance(p.stubs, list) and 'all' in p.stubs):
        p.stubs = list(GENERATORS.keys())
    # Ensure list
    if isinstance(p.stubs, str):
        p.stubs = [p.stubs]
        
    return p


def main():
    args = parse_args()
    log_level = logging.getLevelName((6-args.verbose)*10) if args.verbose != 0 else logging.NOTSET
    logging.basicConfig(level=log_level, format='%(levelname)s: %(message)s')

    # Initialize shared context
    ctx = GeneratorContext.create()

    for stub_name in args.stubs:
        if stub_name not in GENERATORS:
            logging.warning(f"Unknown stub type: {stub_name}")
            continue
            
        logging.info(f"Generating {stub_name} stubs")
        
        spath = Path('autogen') / stub_name
        
        # Instantiate and run generator
        GeneratorClass = GENERATORS[stub_name]
        try:
            gen = GeneratorClass(spath, ctx)
            gen.generate()
        except Exception as e:
            logging.error(f"Failed to generate {stub_name}: {e}", exc_info=True)

if __name__ == '__main__':
    main()

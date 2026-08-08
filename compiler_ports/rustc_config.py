import argparse
from pathlib import Path
import logging
import subprocess
import toml

def create_parser():
    parser = argparse.ArgumentParser(description='Path Argument Parser')
    parser.add_argument('-r', '--rustsrc', type=Path, required=True, help='Path to rust compiler source directory')
    parser.add_argument('-m', '--muslroot', type=Path, required=True, help='Path to musl sysroot directory')
    parser.add_argument('-l', '--llvmbin', type=Path, required=True, help='Path to llvm bin directory')
    parser.add_argument('-t', '--target', type=str, default='wasm32-wali-linux-musl', help='Rustc target name that we are building')
    return parser

def absresolve(p):
    return str(p.resolve())

# Config.toml update
def update_bootstrap_config_toml(rust, muslroot, llvmbin, rtarget):
    bootstrap_config = rust / 'bootstrap.toml'
    with open(bootstrap_config, 'r') as f:
        config = toml.load(f)

    host_platform = subprocess.check_output(f"{llvmbin}/llvm-config --host-target", shell=True, text=True).strip()
    build_config = {
        'target': [host_platform, rtarget]
    }
    wali_config = {
        rtarget: {
            'musl-root': absresolve(muslroot),
            'llvm-libunwind': 'no',
            'crt-static': True,
            'cc': absresolve(llvmbin / 'clang'),
            'cxx': absresolve(llvmbin / 'clang++'),
            'ar': absresolve(llvmbin / 'llvm-ar'),
            'ranlib': absresolve(llvmbin / 'llvm-ranlib')
        },
    }

    if 'build' not in config:
        config['build'] = {}
    config['build'].update(build_config)
    if 'target' not in config:
        config['target'] = {}
    config['target'].update(wali_config)

    logging.info(f"Writing {config} to {bootstrap_config}")

    with open(bootstrap_config, 'w') as f:
        toml.dump(config, f)

def main():
    logging.basicConfig(level=logging.INFO, 
        format='%(levelname)s: %(message)s')
    parser = create_parser()
    args = parser.parse_args()
    rust, muslroot, llvmbin, rtarget = args.rustsrc, args.muslroot, args.llvmbin, args.target
    update_bootstrap_config_toml(rust, muslroot, llvmbin, rtarget)

if __name__ == '__main__':
    main()

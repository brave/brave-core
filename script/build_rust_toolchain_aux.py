import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile

import brave_chromium_utils

# npm run build_rust_toolchain_aux

RUST_BUILD_TOOLS = '//tools/rust'

def back_up_config_toml_template():
    shutil.copyfile('config.toml.template', 'config.toml.template.orig')

def edit_config_toml_template():
    with open('config.toml.template', 'r+') as file:
        updated = re.sub(
            r'^(\[rust\])$',
            r'[target.wasm32-unknown-unknown]\nprofiler = false\n\n\1\nlld = true',
            file.read(),
            flags=re.MULTILINE)
        file.seek(0)
        file.write(updated)

def prepare_run_xpy():
    args = [
        'python3',
        'build_rust.py',
        '--prepare-run-xpy'
    ]
    print(f'Running {" ".join(args)}...')
    subprocess.check_call(args)

def run_xpy():
    with brave_chromium_utils.sys_path(RUST_BUILD_TOOLS):
        import build_rust
        target_triple = build_rust.RustTargetTriple()

    args = [
        'python3',
        'build_rust.py',
        '--run-xpy',
        'build',
        '--build',
        target_triple,
        '--target',
        f'{target_triple},wasm32-unknown-unknown',
        '--stage',
        '1'
    ]
    print(f'Running {" ".join(args)}...')
    subprocess.check_call(args)

def restore_config_toml_template():
    shutil.copy('config.toml.template.orig', 'config.toml.template')

def create_archive():
    with brave_chromium_utils.sys_path(RUST_BUILD_TOOLS):
        import build_rust
        import update_rust
        target_triple = build_rust.RustTargetTriple()
        stage1_output_path = os.path.join(build_rust.RUST_BUILD_DIR, target_triple, 'stage1', 'lib', 'rustlib')
        rust_toolchain = os.path.relpath(update_rust.RUST_TOOLCHAIN_OUT_DIR, brave_chromium_utils.get_src_dir())

        with tarfile.open(f'rust-toolchain-{update_rust.GetRustClangRevision()}.tar.xz', 'w:xz') as tar:
            tar.add(
                os.path.join(stage1_output_path, target_triple, 'bin', 'rust-lld'),
                arcname=os.path.join(rust_toolchain, 'bin', 'rust-lld')
            )
            tar.add(
                os.path.join(stage1_output_path, 'wasm32-unknown-unknown'),
                arcname=os.path.join(rust_toolchain, 'lib', 'rustlib', 'wasm32-unknown-unknown')
            )

def main():
    parser = argparse.ArgumentParser(
        description='Build and package rust-lld and wasm32-unknown-unknown')
    parser.add_argument('--out-dir')
    args = parser.parse_args()
    out_dir = args.out_dir if args.out_dir else os.path.dirname(os.path.realpath(__file__))
    Path(out_dir).mkdir(parents=True, exist_ok=True)

    os.chdir(brave_chromium_utils.wspath(RUST_BUILD_TOOLS))
    back_up_config_toml_template()
    edit_config_toml_template()
    prepare_run_xpy()
    run_xpy()
    restore_config_toml_template()

    os.chdir(out_dir)
    create_archive()



if __name__ == '__main__':
    sys.exit(main())

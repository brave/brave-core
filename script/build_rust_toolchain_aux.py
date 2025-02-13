import os
import re
import shutil
import subprocess
import sys
import tarfile

import brave_chromium_utils

# npm run build_rust_toolchain_aux

TOOLS_RUST = '//tools/rust'

with brave_chromium_utils.sys_path('//tools/rust'):
    import build_rust
    target_triple = build_rust.RustTargetTriple()
    rust_build_dir = build_rust.RUST_BUILD_DIR
    print(build_rust.MakeVersionStamp(build_rust.RUST_REVISION))

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

def create_tar():
    with brave_chromium_utils.sys_path(TOOLS_RUST):
        import build_rust
        import update_rust
        target_triple = build_rust.RustTargetTriple()
        output_path = os.path.join(build_rust.RUST_BUILD_DIR, target_triple, 'stage1', 'lib', 'rustlib')
        rust_toolchain = os.path.relpath(update_rust.RUST_TOOLCHAIN_OUT_DIR, brave_chromium_utils.get_src_dir())

    with tarfile.open(f'rust_toolchain_aux_{target_triple.replace("-", "_")}.gz', 'w:gz') as tar:
        tar.add(
            os.path.join(output_path, target_triple, 'bin', 'rust-lld'),
            arcname=os.path.join(rust_toolchain, 'bin', 'rust-lld')
        )
        tar.add(
            os.path.join(output_path, 'wasm32-unknown-unknown'),
            arcname=os.path.join(rust_toolchain, 'lib', 'rustlib', 'wasm32-unknown-unknown')
        )

def main():
    os.chdir(brave_chromium_utils.wspath(TOOLS_RUST))
    # back_up_config_toml_template()
    # edit_config_toml_template()
    # prepare_run_xpy()
    # run_xpy()
    # restore_config_toml_template()
    os.chdir(os.path.dirname(os.path.realpath(__file__)))
    create_tar()



if __name__ == '__main__':
    sys.exit(main())

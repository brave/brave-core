from contextlib import contextmanager
import os
import re
import shutil
import subprocess
import sys
import tarfile

import brave_chromium_utils

# npm run build_rust_toolchain_aux

@contextmanager
def cwd(cwd):
    oldcwd = os.getcwd()
    os.chdir(cwd)
    try:
        yield
    finally:
        os.chdir(oldcwd)

with brave_chromium_utils.sys_path('//tools/rust'):
    import build_rust
    target_triple = build_rust.RustTargetTriple()
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

# def install_artifacts(rust_lld_output_path, rust_lld_install_path, wasm32_unknown_unknown_output_path, wasm32_unknown_unknown_install_path):
#     shutil.copy2(rust_lld_output_path, rust_lld_install_path)
#     shutil.copytree(wasm32_unknown_unknown_output_path, wasm32_unknown_unknown_install_path)

def create_tar(rust_lld, wasm32_unknown_unknown, rust_lld_install_path, wasm32_unknown_unknown_install_path):
    with tarfile.open(f'rust_toolchain_aux_{target_triple.replace("-", "_")}.gz', 'w:gz') as tar:
        tar.add(rust_lld, arcname=rust_lld_install_path)
        tar.add(wasm32_unknown_unknown, arcname=wasm32_unknown_unknown_install_path)

def main():
    with cwd(os.path.join('tools', 'rust')):
        # back_up_config_toml_template()
        # edit_config_toml_template()
        # prepare_run_xpy()
        # run_xpy()
        # restore_config_toml_template()
        print(os.getcwd())

    output_path = os.path.join('third_party', 'rust-src', 'build', target_triple, 'stage1', 'lib', 'rustlib')
    rust_lld_output_path = os.path.join(output_path, target_triple, 'bin', 'rust-lld')
    wasm32_unknown_unknown_output_path = os.path.join(output_path, 'wasm32-unknown-unknown')

    install_path = os.path.join('third_party', 'rust-toolchain')
    rust_lld_install_path = os.path.join(install_path, 'bin', 'rust-lld')
    wasm32_unknown_unknown_install_path = os.path.join(install_path, 'lib', 'rustlib', 'wasm32-unknown-unknown')

    # install_artifacts(rust_lld_output_path, rust_lld_install_path, wasm32_unknown_unknown_output_path, wasm32_unknown_unknown_install_path) 
    # create_tar(
    #     rust_lld_output_path,
    #     wasm32_unknown_unknown_output_path,
    #     rust_lld_install_path,
    #     wasm32_unknown_unknown_install_path
    # )



if __name__ == '__main__':
    sys.exit(main())

import argparse
import glob
import os
import shutil
import re
import subprocess
import sys
import tarfile

# Updates/adds new crates under brave/tools/crates.
# Call it via `npm run`, e.g.:
# npm run update_brave_tools_crates -- --crate_path=wasm_pack

# This script works based on the contents of the crate's README.chromium file, e.g.:
# Name: wasm-pack
# URL: https://crates.io/crates/wasm-pack (https://github.com/rustwasm/wasm-pack)
# Version: v0.13.1
# will clone "https://github.com/rustwasm/wasm-pack" at tag "v0.13.1" and assemble "wasm-pack" with cargo.

# After the GitHub URL, use the
# `| <path-to-manifest>`
# notation if the desired crate is not in the root of the repo, e.g.:
# URL: https://crates.io/crates/wasm-bindgen (https://github.com/rustwasm/wasm-bindgen | crates/cli)

# Additionally, this script will:
# - update the BUILD.gn file (if one is present in `--crate_path`) with the new version,
# - update the README.chromium file with the new revision.

temp_dir = 'temp'


def parse_args():
    parser = argparse.ArgumentParser(
        description='Updates/adds new crates under brave/tools/crates.')
    parser.add_argument('--crate_path', required=True)
    return parser.parse_args()


def get_file_system_entry(pattern):
    entries = glob.glob(pattern)
    if not entries:
        return None
    elif len(entries) != 1:
        raise Exception(
            f'{pattern} yields multiple entries ({", ".join(entries)})!')
    return entries[0]


def remove_dir(pattern):
    entry = get_file_system_entry(pattern)
    if entry is None:
        return
    print(f'Removing {entry}...')
    shutil.rmtree(entry)


def parse_readme():
    pattern = 'README.chromium'
    readme = get_file_system_entry(pattern)
    if readme is None:
        raise Exception(f'{pattern} was not found!')
    print(f'Parsing {pattern}...')
    fields = ['Name:', 'URL:', 'Version:']
    with open(readme) as file:
        return {
            key.lower(): value.strip()
            for key, value in [
                line.split(': ') for line in file
                if any(field in line for field in fields)
            ]
        }


def get_crate_info():
    readme = parse_readme()
    match = re.match(
        r'^[^(]*?\((?P<github_url>https:\/\/github.com[^()|]*?)(?:\|(?P<manifest_path>[^|]+?))?\)$',
        readme['url'].replace(' ', ''))
    if not match:
        raise Exception("Couldn't parse URL field!")
    return readme | match.groupdict()


def run_git_clone(url, tag):
    args = [
        'git', 'clone', '--recurse-submodules', '--depth=1', f'--branch={tag}',
        url, temp_dir
    ]
    print(f'Running {" ".join(args)}...')
    subprocess.check_call(args)


def run_cargo_package(manifest_path, package):
    cargo_bin = os.path.abspath(
        os.path.join(os.environ['RUSTUP_HOME'], 'bin',
                     'cargo' if sys.platform != 'win32' else 'cargo.exe'))
    args = [
        cargo_bin, 'package', '--no-verify',
        f'--manifest-path={os.path.join(temp_dir, *str(manifest_path or "").split("/"), "Cargo.toml")}',
        f'--package={package}'
    ]
    print(f'Running {" ".join(args)}...')
    subprocess.check_call(args)


def extract_crate():
    pattern = os.path.join(temp_dir, 'target', 'package', '*.crate')
    crate_file = get_file_system_entry(pattern)
    if crate_file is None:
        raise Exception(
            f"Couldn't find .crate file based on pattern {pattern}!")
    print(f'Extracting {crate_file}...')
    with tarfile.open(crate_file, 'r:gz') as tar:
        for tarinfo in tar.getmembers():
            tarinfo.name = re.sub(
                r'^.*?(\d+(?:.\d+)*)',
                lambda match: 'v' + '_'.join(match.group(1).split('.')),
                tarinfo.name)
        tar.extractall()


def update_build_gn():
    pattern = 'BUILD.gn'
    build_gn = get_file_system_entry(pattern)
    if build_gn is None:
        return
    pattern = 'v*'
    version = get_file_system_entry(pattern)
    if version is None:
        raise Exception(f'{pattern} was not found!')
    with open(build_gn, 'r+') as file:
        updated_contents = re.sub(r'v\d+(?:_\d+)*', version, file.read())
        file.seek(0)
        file.write(updated_contents)


def update_readme():
    pattern = 'README.chromium'
    readme = get_file_system_entry(pattern)
    if readme is None:
        raise Exception(f'{pattern} was not found!')
    args = ['git', f'-C', temp_dir, 'rev-parse', 'HEAD']
    print(f'Running {" ".join(args)}...')
    revision = subprocess.check_output(args, text=True)
    with open(readme, 'r+') as file:
        updated_contents = re.sub(r'^Revision.*\n?',
                                  f'Revision: {revision}',
                                  file.read(),
                                  flags=re.MULTILINE)
        file.seek(0)
        file.write(updated_contents)


def main():
    args = parse_args()
    os.chdir(
        os.path.join(os.path.dirname(os.path.realpath(__file__)),
                     args.crate_path))
    remove_dir('v*')
    crate_info = get_crate_info()
    run_git_clone(crate_info['github_url'], crate_info['version'])
    run_cargo_package(crate_info['manifest_path'], crate_info['name'])
    extract_crate()
    update_build_gn()
    update_readme()
    remove_dir(temp_dir)


if __name__ == '__main__':
    sys.exit(main())

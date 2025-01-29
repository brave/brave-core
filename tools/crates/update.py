import argparse
import functools
import glob
import os
import shutil
import re
import subprocess

def get_file_system_entry(pattern):
    entries = glob.glob(pattern)
    if not entries:
        return None
    else:
        if len(entries) != 1:
            raise Exception(pattern + " yields multiple entries (" + ', '.join(entries) + ")!")
        else:
            return entries[0]

def remove_crate_dir():
    crate = get_file_system_entry("v*")
    if crate is None:
        return
    try:
        shutil.rmtree(crate)
    except Exception as e:
        raise Exception("Failed to remove " + crate) from e

def get_git_arguments(url_field):
    if match := re.match(r"^[^(]*?\((?P<git_url>https:\/\/github.com[^()|]*?)(?:\|(?P<patterns>[^|]+?))?\)$", url_field.replace(' ', '')):
        return match.groupdict()
    else:
        raise Exception("Couldn't parse URL field!")

def clone(git_arguments, version):
    match = re.match(r"^[^\d]*?(?P<version>[\d.]+)$", version.replace(' ', ''))
    if not match:
        raise Exception("Couldn't parse Version field!")

    version_subfolder = 'v' + '_'.join(match.groupdict()['version'].split('.'))

    args = [
        "git",
        "clone",
        "--no-checkout",
        "--depth=1",
        "--branch=" + version,
        "--filter=tree:0",
        git_arguments['git_url'],
        version_subfolder
    ]
    print('\nRunning ' + ' '.join(args) + '...')
    subprocess.check_call(args = args)

    if patterns := git_arguments['patterns']:
        args = [
            "git",
            "-C",
            version_subfolder,
            "sparse-checkout",
            "set",
            "--no-cone",
            *patterns.split(',')
        ]
        print('\nRunning ' + ' '.join(args) + '...')
        subprocess.check_call(args = args)

    args = [
        "git",
        "-C",
        version_subfolder,
        "checkout"
    ]
    print('\nRunning ' + ' '.join(args) + '...')
    subprocess.check_call(args = args)

def parse_readme():
    pattern = "README.chromium"
    readme = get_file_system_entry(pattern)
    if readme is None:
        raise Exception(pattern + " was not found!")
    fields = ["URL: ", "Version: "]
    return {
        key.lower(): value.strip() for key, value in
        [line.split(': ') for line in open(readme) if any(field in line for field in fields)]
    }

parser = argparse.ArgumentParser(description='Bump version of Rust tools under brave/tools/crates')
parser.add_argument('--tool_path', required=True)
args, wasm_pack_args = parser.parse_known_args()

os.chdir(args.tool_path)
remove_crate_dir()
readme = parse_readme()
git_arguments = get_git_arguments(readme['url'])
clone(git_arguments, readme['version'])

#!/usr/bin/env vpython3

import os
from pathlib import Path
import shutil
import subprocess
import toml

CONFIG_TOML = {
    'source': {
        'crates-io': {
            'replace-with': 'vendored-sources'
        },
        'vendored-sources': {
            'directory': '../vendor'
        }
    }
}

def main():
    os.chdir(os.path.dirname(os.path.realpath(__file__)))
    members = toml.load('Cargo.toml')['workspace']['members']

    shutil.rmtree('vendor', ignore_errors=True)
    [Path(f'{member}/.cargo/config.toml').unlink(missing_ok=True) for member in members]

    subprocess.run(['cargo', 'vendor'], check=True)
    for member in members:
        Path(f'{member}/.cargo').mkdir(exist_ok=True)
        with open(Path(f'{member}/.cargo/config.toml'), 'w') as f:
            toml.dump(CONFIG_TOML, f)


if __name__ == '__main__':
    main()

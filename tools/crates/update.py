#!/usr/bin/env vpython3

import fnmatch
import json
import os
from pathlib import Path
import shutil
import subprocess
import toml

import versions


def setup_workspace():
    # Remove .cargo/config.toml if it exists
    config_path = Path('.cargo/config.toml')
    if config_path.exists():
        config_path.unlink()

    # Remove vendor directory if it exists
    if Path('vendor').exists():
        shutil.rmtree('vendor')

    # Create src directory and empty lib.rs file
    Path('src').mkdir(exist_ok=True)
    Path('src/lib.rs').touch()

    # Write content to Cargo.toml
    cargo_toml = {
        'package': {
            'name': 'brave-tools-crates',
            'version': '0.1.0',
            'edition': '2021'
        }
    }

    with open('Cargo.toml', 'w') as f:
        toml.dump(cargo_toml, f)


def add_dependencies():
    # Add dependencies using cargo add
    dependencies = [
        'cargo-audit',
        'wasm-bindgen-cli',
        'wasm-opt',
        'wasm-pack',
    ]
    dict = vars(versions)
    for dep in dependencies:
        version = dict[f'{dep.replace("-", "_").upper()}_VERSION']
        subprocess.run(['cargo', 'add', f'{dep}@{version}'], check=True)

    # Update dependencies
    subprocess.run(['cargo', 'update'], check=True)

    # Vendor dependencies
    subprocess.run(['cargo', 'vendor'], check=True)


def create_dependency_placeholders():
    patterns = ['winapi-*gnu*', 'windows_*gnu*']

    cargo_lock_path = Path('Cargo.lock')
    with open(cargo_lock_path) as f:
        lock_content = toml.load(f)

    # Convert patterns to regex patterns
    def matches_any_pattern(name):
        return any(fnmatch.fnmatch(name, pattern) for pattern in patterns)

    # Process each package in the lock file
    if 'package' in lock_content:
        for package in lock_content['package']:
            if 'name' in package and matches_any_pattern(package['name']):
                if 'checksum' in package:
                    del package['checksum']
                    print(f"Removed checksum for package: {package['name']}")

    # Write back the modified lock file if changes were made
    with open(cargo_lock_path, 'w') as f:
        toml.dump(lock_content, f)

    vendor_path = Path('vendor')

    for pattern in patterns:
        # Using glob pattern matching for directories
        matching_dirs = list(vendor_path.glob(pattern))

        for dir_path in matching_dirs:
            if dir_path != vendor_path:
                cargo_toml_path = dir_path / 'Cargo.toml'

                if cargo_toml_path.exists():
                    # Read original Cargo.toml
                    with open(cargo_toml_path) as f:
                        original_toml = toml.load(f)

                    name = original_toml.get('package', {}).get('name')
                    version = original_toml.get('package', {}).get('version')

                    # Remove the original directory
                    shutil.rmtree(dir_path)

                    # Create directory structure
                    (dir_path / 'src').mkdir(parents=True)

                    # Create minimal Cargo.toml
                    minimal_toml = {
                        'package': {
                            'name': name,
                            'version': version
                        }
                    }

                    with open(dir_path / 'Cargo.toml', 'w') as f:
                        toml.dump(minimal_toml, f)

                    # Create empty lib.rs
                    (dir_path / 'src' / 'lib.rs').touch()

                    # Create .cargo-checksum.json
                    with open(dir_path / '.cargo-checksum.json', 'w') as f:
                        json.dump({'files': {}}, f)

                    print(f"Created placeholder for: {dir_path}")


def create_cargo_config():
    # Create .cargo directory and write config.toml
    cargo_config = {
        'source': {
            'crates-io': {
                'replace-with': 'vendored-sources'
            },
            'vendored-sources': {
                'directory': 'vendor'
            }
        }
    }

    Path('.cargo').mkdir(exist_ok=True)
    with open('.cargo/config.toml', 'w') as f:
        toml.dump(cargo_config, f)


def main():
    os.chdir(os.path.dirname(os.path.realpath(__file__)))
    setup_workspace()
    add_dependencies()
    create_dependency_placeholders()
    create_cargo_config()


if __name__ == '__main__':
    main()

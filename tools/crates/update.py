#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# update.py vendors the CLI tools in versions.py and their deps.
# Run this script via npm run update_brave_tools_crates
# whenever you need to add a new CLI tool,
# or bump the version of an existing one.

import fnmatch
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import toml

import brave_chromium_utils
import versions

with brave_chromium_utils.sys_path('//tools/rust'):
    import update_rust
    CARGO = os.path.join(update_rust.RUST_TOOLCHAIN_OUT_DIR, 'bin',
                         'cargo' + ('.exe' if sys.platform == 'win32' else ''))
REMOVE_CRATES = ['winapi-*gnu*', 'windows_*gnu*']
FILTER_CHECKSUM_PATTERNS = ['.git', '**/.git']


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
        subprocess.run([CARGO, 'add', f'{dep}@{version}'], check=True)

    # Update dependencies
    subprocess.run([CARGO, 'update'], check=True)

    # Vendor dependencies
    subprocess.run([CARGO, 'vendor'], check=True)


def create_dependency_placeholders():
    cargo_lock_path = Path('Cargo.lock')
    with open(cargo_lock_path) as f:
        lock_content = toml.load(f)

    # Convert patterns to regex patterns
    def matches_any_pattern(name):
        return any(fnmatch.fnmatch(name, pattern) for pattern in REMOVE_CRATES)

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

    for pattern in REMOVE_CRATES:
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


def filter_dependencies():
    """Filter .cargo-checksum.json files and remove unwanted files from fs."""
    vendor_path = Path('vendor')

    def matches_any_filter_pattern(file_path):
        """Check if a file path matches any of the filter patterns."""
        return any(
            fnmatch.fnmatch(file_path, pattern)
            for pattern in FILTER_CHECKSUM_PATTERNS)

    # Count total packages for progress reporting
    all_packages = [d for d in vendor_path.iterdir() if d.is_dir()]
    total_packages = len(all_packages)

    # Iterate through all vendored packages
    for idx, package_dir in enumerate(all_packages, 1):
        print(f"\rFiltering dependencies: {idx}/{total_packages}",
              end='',
              flush=True)

        # Remove matching files from filesystem by walking the directory
        removed_files = []
        for item in package_dir.rglob('*'):
            if item.is_file():
                relative_path = item.relative_to(package_dir)
                if matches_any_filter_pattern(str(relative_path)):
                    try:
                        item.unlink()
                        removed_files.append(str(relative_path))
                    except Exception as e:
                        print(f"\nWarning: Could not remove {item}: {e}")

        # Print removed files on separate lines if any were removed
        if removed_files:
            print()  # Move to new line
            for file_path in removed_files:
                print(f"  Removed {package_dir.name}/{file_path}")

        # Update checksum file if it exists
        checksum_file = package_dir / '.cargo-checksum.json'
        if not checksum_file.exists():
            continue

        # Read the checksum file
        with open(checksum_file, 'r') as f:
            checksum_data = json.load(f)

        if 'files' not in checksum_data:
            continue

        # Filter out entries matching the patterns from checksum
        original_count = len(checksum_data['files'])
        filtered_files = {
            file_path: checksum
            for file_path, checksum in checksum_data['files'].items()
            if not matches_any_filter_pattern(file_path)
        }

        removed_count = original_count - len(filtered_files)
        if removed_count > 0:
            checksum_data['files'] = filtered_files

            # Write back the filtered checksum file
            with open(checksum_file, 'w') as f:
                json.dump(checksum_data, f)

            if not removed_files:
                print()  # Move to new line if we haven't already
            print(
                f"  Filtered {removed_count} entries from {package_dir.name}/.cargo-checksum.json"
            )

    # Final newline after all packages processed
    print()


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
    filter_dependencies()
    create_dependency_placeholders()
    create_cargo_config()


if __name__ == '__main__':
    main()

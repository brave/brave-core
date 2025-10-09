#!/usr/bin/env python3
"""
Generate README.chromium files for vendored Rust crates.
"""

import os
import re
from pathlib import Path


def parse_cargo_toml(cargo_path):
    """Parse basic information from a Cargo.toml file."""
    with open(cargo_path, 'r') as f:
        content = f.read()

    # Extract package name
    name_match = re.search(r'name\s*=\s*"([^"]+)"', content)
    name = name_match.group(1) if name_match else None

    # Extract version
    version_match = re.search(r'version\s*=\s*"([^"]+)"', content)
    version = version_match.group(1) if version_match else None

    # Extract license
    license_match = re.search(r'license\s*=\s*"([^"]+)"', content)
    license_text = license_match.group(1) if license_match else None

    # Extract description
    description_match = re.search(r'description\s*=\s*"([^"]+)"', content)
    description = description_match.group(1) if description_match else None

    return name, version, license_text, description


def find_license_file(vendor_dir, license_text):
    """Find the appropriate license file in the vendor directory."""
    files = os.listdir(vendor_dir)

    # Prefer Apache license if available
    if 'LICENSE-APACHE' in files:
        return 'LICENSE-APACHE'
    elif 'LICENSE-MIT' in files:
        return 'LICENSE-MIT'
    elif 'LICENSE' in files:
        return 'LICENSE'
    elif 'COPYING' in files:
        return 'COPYING'
    elif 'LICENSE.md' in files:
        return 'LICENSE.md'

    # Look for any file starting with LICENSE
    for f in files:
        if f.startswith('LICENSE'):
            return f

    return None


def generate_readme_chromium(vendor_dir):
    """Generate README.chromium for a vendored crate."""
    cargo_path = os.path.join(vendor_dir, 'Cargo.toml')

    if not os.path.exists(cargo_path):
        print(f"Skipping {vendor_dir}: No Cargo.toml found")
        return False

    # Check if README.chromium already exists
    readme_path = os.path.join(vendor_dir, 'README.chromium')
    if os.path.exists(readme_path):
        return False

    # Parse Cargo.toml
    name, version, license_text, description = parse_cargo_toml(cargo_path)

    if not name or not version:
        print(f"Skipping {vendor_dir}: Could not parse name or version")
        return False

    # Find license file
    license_file = find_license_file(vendor_dir, license_text)

    if not license_file:
        print(f"Warning: No license file found for {name}")
        license_file_line = ""
    else:
        vendor_name = os.path.basename(vendor_dir)
        license_file_line = f"License File: //brave/ui/webui/resources/wasm/vendor/{vendor_name}/{license_file}\n"

    # Generate README.chromium content
    content = f"Name: {name}\n"
    content += f"URL: https://crates.io/crates/{name}\n"
    content += f"Version: {version}\n"

    if license_text:
        content += f"License: {license_text}\n"

    if license_file_line:
        content += license_file_line

    if description:
        content += f"Description: {description}\n"

    # Write README.chromium
    with open(readme_path, 'w') as f:
        f.write(content)

    return True


def main():
    vendor_root = Path(__file__).parent / 'vendor'

    if not vendor_root.exists():
        print(f"Vendor directory not found: {vendor_root}")
        return

    # Process all subdirectories
    created_count = 0
    skipped_count = 0

    for vendor_dir in sorted(vendor_root.iterdir()):
        if not vendor_dir.is_dir():
            continue

        if generate_readme_chromium(vendor_dir):
            created_count += 1
            print(f"Created: {vendor_dir.name}/README.chromium")
        else:
            skipped_count += 1

    print(f"\nSummary:")
    print(f"  Created: {created_count}")
    print(f"  Skipped: {skipped_count}")


if __name__ == '__main__':
    main()

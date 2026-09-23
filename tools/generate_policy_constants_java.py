#!/usr/bin/env python3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Generates Java constants file from Brave policy YAML definitions."""

import argparse
import os
import re
import sys
import zipfile


def extract_policy_name_from_filename(filename):
    """Extract policy name from YAML filename.

    Example: 'BraveWebDiscoveryEnabled.yaml' -> 'BraveWebDiscoveryEnabled'
    """
    basename = os.path.basename(filename)
    if basename.endswith('.yaml'):
        return basename[:-5]  # Remove .yaml extension
    return basename


def find_policy_yaml_files(policy_dir):
    """Find all YAML policy definition files."""
    yaml_files = []
    if not os.path.isdir(policy_dir):
        print(f"Error: Policy directory not found: {policy_dir}",
              file=sys.stderr)
        return yaml_files

    for filename in os.listdir(policy_dir):
        if filename.endswith('.yaml') and filename != '.group.details.yaml':
            yaml_files.append(os.path.join(policy_dir, filename))

    return sorted(yaml_files)


def generate_java_constant_name(policy_name):
    """Convert policy name to Java constant name.

    Handles acronyms properly:
    - 'BraveWebDiscoveryEnabled' -> 'BRAVE_WEB_DISCOVERY_ENABLED'
    - 'BraveP3AEnabled' -> 'BRAVE_P3A_ENABLED'
    - 'BraveAIChatEnabled' -> 'BRAVE_AI_CHAT_ENABLED'
    - 'BraveVPNDisabled' -> 'BRAVE_VPN_DISABLED'
    """
    result = []
    i = 0
    while i < len(policy_name):
        char = policy_name[i]

        if char.isupper():
            # Check if this starts an acronym (consecutive uppercase letters
            # possibly followed by a digit)
            acronym_end = i + 1
            while acronym_end < len(policy_name):
                next_char = policy_name[acronym_end]
                if next_char.isupper():
                    acronym_end += 1
                elif next_char.isdigit():
                    # Include digits in acronyms (like P3A)
                    acronym_end += 1
                else:
                    break

            acronym_len = acronym_end - i

            if acronym_len > 1:
                # We have an acronym
                # Check if the last uppercase letter starts the next word
                if (acronym_end < len(policy_name)
                        and policy_name[acronym_end - 1].isupper()
                        and policy_name[acronym_end].islower()):
                    # The last uppercase is part of next word (e.g., "AIChat")
                    acronym_end -= 1
                    acronym_len -= 1

                if result:
                    result.append('_')
                result.append(policy_name[i:acronym_end].upper())
                i = acronym_end
            else:
                # Single uppercase letter - start of a new word
                if result:
                    result.append('_')
                result.append(char.upper())
                i += 1
        else:
            result.append(char.upper())
            i += 1

    return ''.join(result)


def generate_java_content(policy_names):
    """Generate Java file content with policy constants."""
    java_content = (
        """/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.policy;

/**
 * Policy key constants for Brave policies.
 *
 * <p>This file is auto-generated from policy YAML definitions.
 * Do not edit manually.
 */
public final class BravePolicyConstants {
    private BravePolicyConstants() {
        // Prevent instantiation
    }

""")

    # Generate constants for each policy
    for policy_name in sorted(policy_names):
        constant_name = generate_java_constant_name(policy_name)
        java_content += (f'    public static final String {constant_name} = '
                         f'"{policy_name}";\n')

    java_content += "}\n"
    return java_content


def generate_srcjar(policy_names, output_srcjar):
    """Generate srcjar (zip file) with Java constants."""
    java_content = generate_java_content(policy_names)

    # Create srcjar (zip file) with the Java file at the correct path
    java_path = "org/chromium/chrome/browser/policy/BravePolicyConstants.java"

    os.makedirs(os.path.dirname(output_srcjar), exist_ok=True)
    with zipfile.ZipFile(output_srcjar, 'w', zipfile.ZIP_DEFLATED) as zf:
        zf.writestr(java_path, java_content.encode('utf-8'))

    print(f"Generated {len(policy_names)} policy constants in srcjar "
          f"{output_srcjar}")


def main():
    parser = argparse.ArgumentParser(
        description='Generate Java constants from Brave policy YAML files')
    parser.add_argument('--policy-dir',
                        required=True,
                        help='Directory containing policy YAML files')
    parser.add_argument('--output-srcjar',
                        required=True,
                        help='Output srcjar file path')

    args = parser.parse_args()

    # Find all YAML policy files
    yaml_files = find_policy_yaml_files(args.policy_dir)

    if not yaml_files:
        print(f"Warning: No YAML policy files found in {args.policy_dir}",
              file=sys.stderr)
        sys.exit(1)

    # Extract policy names from filenames
    policy_names = []
    for yaml_file in yaml_files:
        policy_name = extract_policy_name_from_filename(yaml_file)
        if policy_name:
            policy_names.append(policy_name)

    # Generate srcjar output
    generate_srcjar(policy_names, args.output_srcjar)

    return 0


if __name__ == '__main__':
    sys.exit(main())

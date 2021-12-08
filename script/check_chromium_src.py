#!/usr/bin/env python3
#
# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */
#
# This script looks for problems in chromium_src.
#
# 1. Look for files in chromium_src that don't have a corresponding
#    file under src tree (or grit).
#
# 2. Look for #defines that redefine symbols that aren't present in
#    the overridden file.
#
# 3. Look for #include statements of the overridden file that have
#    incorrect number of ../ in the path.
#
# !!! This script does return false positives, but better check a
# !!! few of those than not check at all.

import argparse
import re
import os
import sys

# Look for potential problems in chromium_src overrides.

BRAVE_SRC = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
BRAVE_CHROMIUM_SRC = os.path.join(BRAVE_SRC, 'chromium_src')
CHROMIUM_SRC = os.path.abspath(os.path.dirname(BRAVE_SRC))

# pylint: disable-next=line-too-long
NORMAL_DEFINITIONS_REGEXP = r'#define[\s\\]+([a-zA-Z0-9_]+[^\s\(]*)(?:[ \t]+\\\s*|[ \t])+([a-zA-Z0-9_]+[^\s\(]*)'
# pylint: disable-next=line-too-long
FUNCTION_LIKE_DEFINITIONS_REGEXP = r'#define[\s\\]+([a-zA-Z0-9_]+)[\s\\]*\(.*?\)(?:[ \t]+\\\s*|[ \t])([a-zA-Z0-9_]*[\s\\]*\(.*?\))'

EXCLUDES = [
    '.*/BUILD.gn',
    '.*/DEPS',
    '.*/sources.gni',
    'CPPLINT.cfg',
    '_(unit|browser)test(_mac)?.(cc|mm)',
    'base/feature_override.h',
    'chrome/installer/linux/common/brave-browser/chromium-browser.appdata.xml',
    'chrome/installer/linux/common/brave-browser/chromium-browser.info',
    'content/browser/tld_ephemeral_lifetime.cc',
    'content/public/browser/tld_ephemeral_lifetime.h',
    'third_party/blink/renderer/core/origin_trials/origin_trials.cc',
    'third_party/blink/renderer/modules/battery/navigator_batterytest.cc',
    'third_party/blink/renderer/modules/bluetooth/navigator_bluetoothtest.cc',
    'third_party/blink/renderer/modules/quota/navigator_storagetest.cc',
    'third_party/blink/renderer/modules/storage/brave_dom_window_storage.h',
]

GRIT_EXCLUDES = [
    '.*/DEPS'
]

GRIT_INCLUDES = [
    'third_party/blink/renderer/core/origin_trials/origin_trials.cc'
]


def do_check_includes(override_filepath):
    """
    Warns if |override_filepath| uses relative includes and, if so, also checks
    them for the correct number of ".." in the include statement for the
    original file.
    """
    with open(override_filepath, mode='r', encoding='utf-8') as override_file:
        normalized_override_filepath = override_filepath.replace('\\', '/')
        override_filename = os.path.basename(override_filepath)
        override_dirpath = os.path.dirname(override_filepath)

        # Calculate the expected number of parent dirs.
        # -1 for the dir where the file is.
        # +3 to get to chromium_src->brave->src.
        expected_count = len(override_dirpath.split(os.path.sep)) - 1 + 3

        for line in override_file:
            # Check src/-prefixed includes
            regexp = r'^#include "src/(.*)"'
            line_match = re.search(regexp, line)
            if line_match:
                if line_match.group(1) != normalized_override_filepath:
                    print(f"WARNING: {override_filepath} uses a src/-prefixed" +
                          " include that doesn't point to the expected file:")
                    print(f"         Include: {line}" +
                          "         Expected include target: src/" +
                          f"{normalized_override_filepath}")
                    print("-------------------------")
                continue

            # Check relative includes go up the expected amount of steps.
            # We're only interested in relative include paths.
            regexp = rf'^#include "(\.\./.*{override_filename})"'
            line_match = re.search(regexp, line)
            if not line_match:
                continue

            print(f"WARNING: {override_filepath} uses a relative include:\n" +
                  f"         {line}" +
                  "         Switch to using a src/-prefixed include instead.")
            print("-------------------------")

            # Count the number of '../' elements, but don't use the OS's path
            # separator here, since we're counting paths from a C++ include.
            include_path = line_match.group(1)
            actual_count = include_path.split('/').count('..')

            # Check actual vs expected.
            if actual_count != expected_count:
                print(f"ERROR: while processing {override_filepath}")

                # Sanity check
                saved_cwd = os.getcwd()
                os.chdir(override_dirpath)
                if not os.path.isfile(include_path):
                    print(f"       File {include_path} doesn't exist")
                os.chdir(saved_cwd)

                print(f"       Expected {expected_count} \"..\"")
                print(f"       Found {actual_count} \"..\"")
                print("-------------------------")


def do_check_defines(override_filepath, original_filepath):
    """
    Finds `#define TARGET REPLACEMENT` statements in |override_filepath| and
    attempts to find the <TARGET> in the |original_filepath|.
    """
    with open(override_filepath, mode='r', encoding='utf-8') as override_file:
        content = override_file.read()
        matches = []

        # Search for all matches for normal definitions
        # e.g. #define FooBar FooBar_ChromiumImpl
        normal_matches = re.findall(NORMAL_DEFINITIONS_REGEXP, content)
        matches += [{'value': m, 'is_func': False} for m in normal_matches]

        # Search for all matches for function-like definitions
        # e.g. #define FooBar(P1, P2) FooBar_ChromiumImpl(P1, P2, p3)
        function_matches = re.findall(FUNCTION_LIKE_DEFINITIONS_REGEXP,
                                      content, flags=re.DOTALL)
        matches += [{'value': m, 'is_func': True} for m in function_matches]

        if not matches:
            return

        for match in matches:
            target = match['value'][0]
            replacement = match['value'][1]

            if not replacement:
                continue

            # Adjust target name for BUILDFLAG_INTERNAL_*() cases for
            # function-like matches.
            if match['is_func'] and target.startswith('BUILDFLAG_INTERNAL_'):
                buildflag_match = re.search(
                    r'BUILDFLAG_INTERNAL_(\S*)', target)
                target = buildflag_match.group(1)

            # Report ERROR if target can't be found in the original file.
            with open(original_filepath, mode='r', encoding='utf-8') as \
                original_file:
                if target not in original_file.read():
                    print(f"ERROR: Unable to find symbol {target}" +
                          f" in {original_filepath}")
                    print("-------------------------")


def do_check_overrides(overrides_list, search_dir, check_includes=False):
    """
    Checks that each path in the passed in list |overrides_list| exists in
    the passed in directory (|search_dir|), optionally checking includes too.
    """
    print("--------------------------------------------------")
    print(f"Checking overrides in {search_dir} ...")
    print("--------------------------------------------------")
    for override_filepath in overrides_list:
        original_filepath = os.path.join(search_dir, override_filepath)
        if not os.path.isfile(original_filepath):
            print(f"WARNING: No source for override {override_filepath}")
            print("-------------------------")
            continue

        do_check_defines(override_filepath, original_filepath)
        if check_includes:
            do_check_includes(override_filepath)


def get_generated_builddir(build, target_os=None, target_arch=None):
    """
    Returns the path to the generated build directory for a |build|, accounting
    for the optional |target_os| and |target_arch| parameters, if present.
    """
    build_dirname = '_'.join(filter(None,
                                    [target_os, build, target_arch]))
    return os.path.join(CHROMIUM_SRC, 'out', build_dirname, 'gen')


def filter_chromium_src_filepaths(include_regexp=None, exclude_regexp=None):
    """
    Return a list of paths pointing to the files in |BRAVE_CHROMIUM_SRC|
    after filtering the results according to the |include_regexp| and the
    |exclude_regexp| regexp rules, where |include_regexp| takes precedence
    over |exclude_regexp|.
    """
    result = []

    if not include_regexp and not exclude_regexp:
        return result

    for dir_path, _dirnames, filenames in os.walk(BRAVE_CHROMIUM_SRC):
        for filename in filenames:
            full_path = os.path.join(dir_path, filename)

            # Strip the BRAVE_CHROMIUM_SRC prefix to match against the
            # list of relative paths to be included and/or excluded
            relative_path = full_path.replace(BRAVE_CHROMIUM_SRC, '')[1:]

            # Normalize back slashes to forward slashes just in case we're in
            # Windows before trying to match them against a regular expression.
            normalized_path = relative_path.replace('\\', '/')
            if include_regexp and not re.search(
                include_regexp, normalized_path):
                continue
            if exclude_regexp and re.search(exclude_regexp, normalized_path):
                continue

            # Append the OS-dependant relative path instead of the normalized
            # one as that's what functions using this list of paths expect.
            result.append(relative_path)

    return result


def main(args):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build',
                        help='Build type (i.e. Component|Static|Debug|Release)',
                        nargs='?',
                        default='Component')
    parser.add_argument('--os',
                        required=False,
                        help='Target OS (e.g. android|win|linux|mac|ios)')
    parser.add_argument('--arch',
                        required=False,
                        help='Target architecture (e.g. x86|x64|arm|arm64)')
    options = parser.parse_args(args)

    gen_buildir = get_generated_builddir(options.build,
                                         options.os,
                                         options.arch)

    # Check that the required directories exist.
    for directory in [BRAVE_SRC, gen_buildir]:
        if not os.path.isdir(directory):
            print(f"ERROR: {directory} is not a valid directory.")
            return 1

    # Change into the chromium_src directory for convenience.
    os.chdir(BRAVE_CHROMIUM_SRC)

    # Check non-GRIT overrides.
    src_overrides = filter_chromium_src_filepaths(
        exclude_regexp='|'.join(EXCLUDES + ['python_modules|.*grit.*']))
    do_check_overrides(src_overrides, CHROMIUM_SRC, True)

    # Check GRIT overrides.
    grit_overrides = filter_chromium_src_filepaths(
        include_regexp='|'.join(GRIT_INCLUDES + ['.*grit.*']),
        exclude_regexp='|'.join(GRIT_EXCLUDES))
    do_check_overrides(grit_overrides, gen_buildir, False)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

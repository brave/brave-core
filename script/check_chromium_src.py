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

# pylint: disable=line-too-long
NORMAL_DEFINITIONS_REGEXP = r'^#define[\s\\]+([a-zA-Z0-9_]+[^\s\(]*)(?:[ \t]+\\\s*|[ \t])?([a-zA-Z0-9_]+[^\s\(]*)?$'
FUNCTION_LIKE_DEFINITIONS_REGEXP = r'^#define[\s\\]+([a-zA-Z0-9_]+)[\s\\]*\(.*?\)(?:[ \t]+\\\s*|[ \t])?([a-zA-Z0-9_]*[\s\\]*\(.*?\))?'
# pylint: enable=line-too-long

RE_EXCLUDES = [
    '.*\.cfg',
    '.*\.clangd',
    '.*\.gn',
    '.*\.gni',
    '.*\.info',
    '.*\.xml',
    '.*/DEPS',
]

# Please, keep in alphabetical case-insensitive order
# pylint: disable=line-too-long
PATH_EXCLUDES = [
    'base/feature_override.h',
    'chrome/browser/devtools/url_constants_unittest.cc',
    'chrome/browser/history/history_utils_unittest.cc',
    'chrome/browser/notifications/notification_handler_impl.h',
    'chrome/browser/safe_browsing/download_protection/check_client_download_request_base_browsertest.cc',
    'chrome/browser/shell_integration_unittest_mac.cc',
    'chrome/browser/signin/account_consistency_disabled_unittest.cc',
    'chrome/browser/ui/autofill/payments/brave_save_card_bubble_controller_impl_browsertest.cc',
    'chrome/browser/ui/views/tabs/tab_hover_card_bubble_view_browsertest.cc',
    'chrome/common/chrome_constants_unittest_mac.cc',
    'chrome/installer/mini_installer/brave_mini_installer_unittest.cc',
    'chrome/installer/setup/brave_behaviors.cc',
    'chrome/install_static/brave_install_details_unittest.cc',
    'chrome/install_static/brave_install_modes_unittest.cc',
    'chrome/install_static/brave_install_util_unittest.cc',
    'chrome/install_static/brave_product_install_details_unittest.cc',
    'chrome/install_static/brave_user_data_dir_win_unittest.cc',
    'components/history/core/browser/sync/brave_typed_url_sync_bridge_unittest.cc',
    'components/history/core/browser/sync/chromium_typed_url_sync_bridge_unittest.cc',
    'components/privacy_sandbox/privacy_sandbox_settings_unittest.cc',
    'components/search_engines/brave_template_url_prepopulate_data_unittest.cc',
    'components/search_engines/brave_template_url_service_util_unittest.cc',
    'components/variations/service/field_trial_unittest.cc',
    'net/base/brave_proxy_string_util_unittest.cc',
    'net/cookies/brave_canonical_cookie_unittest.cc',
    'third_party/blink/renderer/modules/storage/brave_dom_window_storage.h',
    'v8/include/DO NOT PUT FILES HERE',
]
# pylint: enable=line-too-long

GRIT_EXCLUDES = [
    '.*/DEPS',
    '.*\.py',
]

GRIT_INCLUDES = []


def do_check_includes(override_filepath):
    """
    Errors if |override_filepath| uses relative includes and also checks
    src/ and ../gen-prefixed includes for naming consistency between the
    original and the override.
    """
    error_count = 0
    with open(override_filepath, mode='r', encoding='utf-8') as override_file:
        normalized_override_filepath = override_filepath.replace('\\', '/')
        override_filename = os.path.basename(override_filepath)

        for line in override_file:
            # Check src/-prefixed includes
            regexp = r'^#include "src/(.*)"'
            line_match = re.search(regexp, line)
            if line_match:
                if line_match.group(1) != normalized_override_filepath:
                    # Check for v8 overrides, they can have includes starting
                    # with src.
                    if normalized_override_filepath.startswith("v8/src"):
                        continue
                    print(f"ERROR: {override_filepath} uses a src/-prefixed" +
                          " include that doesn't point to the expected file:")
                    print(f"       Include: {line}" +
                          "       Expected include target: src/" +
                          f"{normalized_override_filepath}")
                    print("-------------------------")
                    error_count += 1
                continue
            gen_regexp = r'^#include "../gen/(.*)"'
            line_match = re.search(gen_regexp, line)
            if line_match:
                if line_match.group(1) != normalized_override_filepath:
                    print(f"ERROR: {override_filepath} uses a ../gen/" +
                          "-prefixed include that doesn't point to the " +
                          "expected file:")
                    print(f"       Include: {line}" +
                          "       Expected include target: ../gen/" +
                          f"{normalized_override_filepath}")
                    print("-------------------------")
                    error_count += 1
                continue

            # Check for relative includes.
            regexp = rf'^#include "(\.\./.*{override_filename})"'
            line_match = re.search(regexp, line)
            if not line_match:
                continue

            print(f"ERROR: {override_filepath} uses a relative include:\n" +
                  f"       {line}" +
                  "       Switch to using a src/-prefixed include instead.")
            print("-------------------------")
            error_count += 1
    return error_count


def strip_comments(content):
    """
    Strips comments from c++ file content
    """
    MULTILINE_COMMENT_REGEX=r'(/\*([\s\S]*?)\*/)'
    ONE_LINE_COMMENT_REGEX=r'(\s*//.*$)'
    stripped = re.sub(MULTILINE_COMMENT_REGEX, '', content, flags=re.MULTILINE)
    return re.sub(ONE_LINE_COMMENT_REGEX, '', stripped, flags=re.MULTILINE)


def validate_define(override_filepath, content, target):
    """
    Validates that defined symbol has a corresponding #undef and checks if the
    symbol is used internally in the override file.
    """
    found = {'def_position': -1, 'undef_position': False, 'other': False}
    pattern = rf'^(.*\b{target}\b.*)$'
    matches = re.finditer(pattern, content, re.MULTILINE)
    for match in matches:
        line = match.group()
        if re.match(rf'^#define[\s\\]+{target}', line):
            # Found #define of the target
            found['def_position'] = match.start()
        elif (re.match(rf'^#undef[\s\\]+{target}', line) and
               found['def_position'] >= 0 and
                 match.start() > found['def_position']):
            # Found #undef of the target
            found['undef_position'] = True
        else:
            # Found internal use of the target
            found['other'] = True

    error_count = 0
    if found['def_position'] == -1:
        print(f"SCRIPT ERROR: Expected to find #define {target} in " +
              f"{override_filepath}.")
        print("-------------------------")
        error_count = 1
    if not found['undef_position']:
        print(
            f"ERROR: Expected to find #undef {target} in {override_filepath}.")
        if override_filepath.endswith('.h'):
            print("       If this symbol is intended to propagate beyond this" +
                  " header then add it to exceptions in this script.")
        print("-------------------------")
        error_count += 1
    return (error_count, found['other'])


def is_header_guard_define(override_filepath, target):
    """
    Checks if the target define is the header guard for the override file path
    """
    guard_string = ("BRAVE_CHROMIUM_SRC_" +
                    override_filepath.
                    replace('\\', '/').
                    replace('/', '_').
                    replace('-', '_').
                    replace('.', '_').
                    upper() + "_")
    return target == guard_string


def do_check_defines(override_filepath, original_filepath):
    """
    Finds `#define TARGET REPLACEMENT` statements in |override_filepath| and
    attempts to find the <TARGET> in the |original_filepath|.
    """
    error_count = 0
    with open(override_filepath, mode='r', encoding='utf-8') as override_file:
        content = strip_comments(override_file.read())
        matches = []

        # Search for all matches for normal definitions
        # e.g. #define FooBar FooBar_ChromiumImpl
        normal_matches = re.findall(NORMAL_DEFINITIONS_REGEXP, content,
                                    flags=re.MULTILINE)
        matches += [{'value': m, 'is_func': False} for m in normal_matches]

        # Search for all matches for function-like definitions
        # e.g. #define FooBar(P1, P2) FooBar_ChromiumImpl(P1, P2, p3)
        function_matches = re.findall(FUNCTION_LIKE_DEFINITIONS_REGEXP,
                                      content, flags=re.MULTILINE)
        matches += [{'value': m, 'is_func': True} for m in function_matches]

        if not matches:
            return error_count

        for match in matches:
            target = match['value'][0]

            # Skip header guard defines
            if (override_filepath.endswith(".h") and
                 is_header_guard_define(override_filepath, target)):
                continue

            err_count, used_internally = validate_define(
                override_filepath, content, target)
            error_count += err_count

            # Adjust target name for BUILDFLAG_INTERNAL_*() cases for
            # function-like matches.
            if match['is_func'] and target.startswith('BUILDFLAG_INTERNAL_'):
                buildflag_match = re.search(
                    r'BUILDFLAG_INTERNAL_(\S*)', target)
                target = buildflag_match.group(1)

            # Report ERROR if target can't be found in the original file.
            with open(original_filepath, mode='r', encoding='utf-8') as \
                original_file:
                if target not in strip_comments(original_file.read()):
                    if not used_internally:
                        print(f"ERROR: Unable to find symbol {target}" +
                              f" in {original_filepath}")
                        print("-------------------------")
                        error_count += 1
                    else:
                        print(f"INFO: Ignoring symbol {target}:")
                        print("      Symbol is used internally in " +
                              "chromium_src/" +
                              override_filepath.replace('\\', '/'))
                        print(f"      Symbol is NOT found in " +
                              f"{original_filepath}.")
                        print("-------------------------")
    return error_count


def do_check_overrides(overrides_list,
                       search_dir,
                       gen_dir=None):
    """
    Checks that each path in the passed in list |overrides_list| exists in
    the passed in directory (|search_dir|), optionally checking includes too.
    """
    error_count = 0
    print("--------------------------------------------------")
    print(f"Checking overrides in {search_dir} ...")
    print("--------------------------------------------------")
    for override_filepath in overrides_list:
        original_filepath_found = False
        original_filepath = os.path.join(search_dir, override_filepath)
        if not os.path.isfile(original_filepath):
            if gen_dir is not None:
                gen_filepath = os.path.join(gen_dir, override_filepath)
                if os.path.isfile(gen_filepath):
                    original_filepath = gen_filepath
                    original_filepath_found = True
        else:
            original_filepath_found = True
        if not original_filepath_found:
            print(f"ERROR: No source for override {override_filepath}")
            print("       If this is not a true override, then add the path" +
                  " to the PATH_EXCLUDES in this script.")
            print("       Otherwise, the upstream file is gone and a fix is" +
                  " required.")
            print("-------------------------")
            error_count += 1
            continue

        error_count += do_check_defines(override_filepath, original_filepath)
        error_count += do_check_includes(override_filepath)
    if not error_count:
        print("OK.")
    return error_count


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

            # Append the OS-dependent relative path instead of the normalized
            # one as that's what functions using this list of paths expect.
            result.append(relative_path)

    return result


def validate_exclusions(paths):
    """
    Validate the each path listed in exclusions paths is still a valid path.
    Otherwise the developer who removed the excluded file should also remove
    it from the exclusions list.
    """
    error_count = 0
    print("--------------------------------------------------")
    print("Validating exclusions...")
    print("--------------------------------------------------")
    for path in paths:
        full_path = os.path.join(BRAVE_CHROMIUM_SRC, path).replace('\\', '/')
        if not os.path.exists(full_path):
            print("ERROR: Path listed in exclusions cannot be found: " +
                  f"chromium_src/{path}")
            print("       If the file was removed then also remove it from" +
                  " the exclusions list of this script.")
            print("-------------------------")
            error_count += 1
    if not error_count:
        print("OK.")
    return error_count


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

    # Check non-RE exclusions
    error_count = validate_exclusions(PATH_EXCLUDES)

    # Change into the chromium_src directory for convenience.
    os.chdir(BRAVE_CHROMIUM_SRC)

    # Check non-GRIT overrides.
    src_overrides = filter_chromium_src_filepaths(
        exclude_regexp='|'.join(RE_EXCLUDES + PATH_EXCLUDES + GRIT_INCLUDES +
                                ['python_modules|.*grit.*']))
    error_count += do_check_overrides(src_overrides, CHROMIUM_SRC, gen_buildir)

    # Check GRIT overrides.
    grit_overrides = filter_chromium_src_filepaths(
        include_regexp='|'.join(GRIT_INCLUDES + ['.*grit.*']),
        exclude_regexp='|'.join(GRIT_EXCLUDES))
    error_count += do_check_overrides(grit_overrides, gen_buildir)
    if error_count:
        print("--------------------------------------------------")
        print(f"Found {error_count} error(s).")
    return error_count


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

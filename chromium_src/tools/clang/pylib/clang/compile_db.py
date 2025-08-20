# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import copy
import os
import re

import override_utils

# Remove redirect_cc from the command line.
CMD_PATTERN_TO_REPLACE = (r'rewrapper(\.exe)',
                          r'(rewrapper|redirect_cc)(\.exe)')

_CLANG_WRAPPER_CMD_LINE_RE: object  # Injected from upstream compile_db.py.
assert CMD_PATTERN_TO_REPLACE[0] in _CLANG_WRAPPER_CMD_LINE_RE.pattern
_CLANG_WRAPPER_CMD_LINE_RE = re.compile(
    _CLANG_WRAPPER_CMD_LINE_RE.pattern.replace(*CMD_PATTERN_TO_REPLACE),
    re.VERBOSE)

new_db_entries = []


@override_utils.override_function(globals())
def ProcessCompileDatabase(original_function,
                           compile_db,
                           filtered_args,
                           target_os=None):
    # Handle multiple flags passed as a single comma-separated value.
    if filtered_args and len(filtered_args) == 1 and ',' in filtered_args[0]:
        filtered_args = filtered_args[0].split(',')

    ext_filtered_flags = [
        # Remove clangd-indexer unsupported flags.
        '-gno-codeview-command-line',
        '-Wno-delayed-template-parsing-in-cxx20',
        '-Wno-thread-safety-reference-return',
        '-Wno-c++11-narrowing-const-reference',
    ]

    if filtered_args:
        filtered_args.extend(ext_filtered_flags)
    else:
        filtered_args = ext_filtered_flags

    return original_function(compile_db, filtered_args, target_os=target_os)


@override_utils.override_function(globals())
def _FilterFlags(original_function, command, additional_filtered_flags):
    flags = original_function(command, additional_filtered_flags)
    flags_to_restore = [
        ' -Xclang -fexperimental-max-bitint-width=256',
        ' -Xclang -iquote../../brave/chromium_src',
    ]

    for flag_to_restore in flags_to_restore:
        if flag_to_restore in command and flag_to_restore not in flags:
            flags += flag_to_restore
    return flags


@override_utils.override_function(globals())
def _ProcessEntry(original_function, entry, filtered_args, target_os):
    entry = original_function(entry, filtered_args, target_os)

    entry_file = entry['file']
    brave_chromium_src_file = re.sub(r'^(\.\./\.\./|gen/)',
                                     'brave/chromium_src/', entry_file)
    if brave_chromium_src_file != entry_file:
        rel_brave_chromium_src_file = f'../../{brave_chromium_src_file}'
        abs_brave_chromium_src_file = os.path.join(entry['directory'],
                                                   rel_brave_chromium_src_file)
        if os.path.exists(abs_brave_chromium_src_file):
            orig_entry = copy.deepcopy(entry)
            force_include_flag = '/FI' if _IsTargettingWindows(
                target_os) else '-include'

            src_brave_chromium_src_file = f'src/{brave_chromium_src_file}'
            orig_entry['command'] += (
                f' {force_include_flag} {src_brave_chromium_src_file}')
            new_db_entries.append(orig_entry)

            entry['command'] = entry['command'].replace(
                entry_file, rel_brave_chromium_src_file)
            entry['file'] = rel_brave_chromium_src_file

    return entry


@override_utils.override_function(globals())
def ProcessCompileDatabase(original_function,
                           compile_db,
                           filtered_args,
                           target_os=None):
    compile_db = original_function(compile_db,
                                   filtered_args,
                                   target_os=target_os)

    compile_db.extend(new_db_entries)
    return compile_db

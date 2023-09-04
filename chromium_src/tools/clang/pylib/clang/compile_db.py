# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import os
import re

import override_utils

# Remove redirect_cc from the command line.
GOMACC_PATTERN_TO_REPLACE = (r'gomacc(\.exe)', r'(gomacc|redirect_cc)(\.exe)')

# pylint: disable=used-before-assignment
assert GOMACC_PATTERN_TO_REPLACE[0] in _GOMA_CMD_LINE_RE.pattern
_GOMA_CMD_LINE_RE = re.compile(
    _GOMA_CMD_LINE_RE.pattern.replace(*GOMACC_PATTERN_TO_REPLACE))


@override_utils.override_function(globals())
def _FilterFlags(original_function, command, additional_filtered_flags):
    filtered_flags = [
        # Remove clangd-indexer unsupported flag.
        '-gno-codeview-command-line'
    ]
    if additional_filtered_flags:
        additional_filtered_flags.extend(filtered_flags)
    else:
        additional_filtered_flags = filtered_flags

    flags = original_function(command, additional_filtered_flags)
    flags_to_restore = [
        # Clangd 15+ is required, VSCode extension includes it.
        ' -Xclang -fexperimental-max-bitint-width=256',
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
                                     '../../brave/chromium_src/', entry_file)
    if brave_chromium_src_file != entry_file:
        abs_brave_chromium_src_file = os.path.join(entry['directory'],
                                                   brave_chromium_src_file)
        if os.path.exists(abs_brave_chromium_src_file):
            entry['command'] = entry['command'].replace(
                entry_file, brave_chromium_src_file)
            entry['file'] = brave_chromium_src_file

    return entry

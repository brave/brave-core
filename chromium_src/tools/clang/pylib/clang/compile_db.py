# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import re

import override_utils

# Remove redirect_cc from the command line.
GOMACC_PATTERN_TO_REPLACE = ('gomacc(\.exe)', '(gomacc|redirect_cc)(\.exe)')

# pylint: disable=used-before-assignment
assert GOMACC_PATTERN_TO_REPLACE[0] in _GOMA_CMD_LINE_RE.pattern
_GOMA_CMD_LINE_RE = re.compile(
    _GOMA_CMD_LINE_RE.pattern.replace(*GOMACC_PATTERN_TO_REPLACE))


@override_utils.override_function(globals())
def _FilterFlags(original_function, command, additional_filtered_flags):
    flags = original_function(command, additional_filtered_flags)
    if additional_filtered_flags:
        # Use --filter_arg to pass brave-specific args to apply here.
        flags_to_restore = {
            'brave-keep-bitint256':
            ' -Xclang -fexperimental-max-bitint-width=256',
        }
        for external_flag, flag_to_restore in flags_to_restore.items():
            if external_flag in additional_filtered_flags:
                if flag_to_restore in command and flag_to_restore not in flags:
                    flags += flag_to_restore
    return flags

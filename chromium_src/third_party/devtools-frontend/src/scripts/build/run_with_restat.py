# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import json
import os
import re
import sys

import brave_chromium_utils
import override_utils

from response_file import expand_response_files


def ensure_hardlink(src, dst):
    src = os.path.abspath(src) if not os.path.isabs(src) else src
    dst = os.path.abspath(dst) if not os.path.isabs(dst) else dst

    try:
        os.link(src, dst)
    except FileExistsError:
        if not os.path.samefile(src, dst):
            # recreating link if dst is not pointing to the src
            try:
                os.unlink(dst)
                os.link(src, dst)
            except (FileExistsError, FileNotFoundError):
                # Ignore this error, it happens because of a race condition
                # on android when the relevant target is running for more than
                # one architecture. This is a temporary workaround
                # TODO(https://github.com/brave/brave-browser/issues/49768)
                pass


# ts_library_split.gni (used for split_compilation targets) type-checks
# directly via `tsc -p <tsconfig>`, bypassing ts_library.py's `--sources`
# argparse override that hardlinks chromium_src overrides in as `*.patch.ts`
# (see //brave/docs/devtools_frontend_patching.md). Without it, tsc can't
# resolve a `*.patch.js` import added by a chromium_src override, so replicate
# the hardlinking here from the tsconfig's own `files` list.
def hardlink_chromium_src_overrides(tsconfig_path):
    tsconfig_dir = os.path.dirname(tsconfig_path)
    with open(tsconfig_path, encoding='utf8') as f:
        tsconfig = json.load(f)

    for rel_file in tsconfig.get('files', []):
        if not rel_file.endswith('.ts') or rel_file.endswith('.d.ts'):
            continue
        abs_file = os.path.abspath(os.path.join(tsconfig_dir, rel_file))
        override = brave_chromium_utils.get_chromium_src_override(abs_file)
        if os.path.exists(override):
            ensure_hardlink(override, re.sub(r'\.ts$', '.patch.ts', abs_file))


@override_utils.override_function(globals())
def main(original_function):
    args = expand_response_files(sys.argv[1:])
    if '--' in args:
        command_args = args[args.index('--') + 1:]
        if '-p' in command_args:
            hardlink_chromium_src_overrides(
                command_args[command_args.index('-p') + 1])
    return original_function()

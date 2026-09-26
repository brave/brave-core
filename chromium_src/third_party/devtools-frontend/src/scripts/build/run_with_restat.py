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

with brave_chromium_utils.sys_path('//brave/tools/typescript'):
    import tsc_timeout_retry
    from chromium_src_hardlink import ensure_hardlink


# Brave's own DevTools sources live in //brave/third_party/devtools-frontend,
# but every file a ts_library target compiles has to sit under that target's
# rootDir, so it is named as if it sat next to the upstream sources it extends
# (see the scripts-build-typescript-typescript.gni patch). Put the real file
# there. See //brave/docs/devtools_frontend_patching.md.
def hardlink_brave_sources(sources):
    src_dir = brave_chromium_utils.get_src_dir()
    for source in sources:
        brave_source = brave_chromium_utils.wspath(
            f'//brave/{os.path.relpath(source, src_dir)}')
        if os.path.exists(brave_source):
            # Unconditionally, so that a leftover copy from a previous
            # mechanism doesn't shadow the real file.
            ensure_hardlink(brave_source, source)


# tsc can't resolve a `*.patch.js` import added by a chromium_src override
# unless the override is visible to it as a sibling `*.patch.ts`, so hardlink
# those in too.
def hardlink_chromium_src_overrides(sources):
    for source in sources:
        if not os.path.exists(source):
            continue
        override = brave_chromium_utils.get_chromium_src_override(source)
        if os.path.exists(override):
            ensure_hardlink(override, re.sub(r'\.ts$', '.patch.ts', source))


def tsconfig_sources(tsconfig_path):
    tsconfig_dir = os.path.dirname(tsconfig_path)
    with open(tsconfig_path, encoding='utf8') as f:
        tsconfig = json.load(f)

    return [
        os.path.abspath(os.path.join(tsconfig_dir, rel_file))
        for rel_file in tsconfig.get('files', [])
        if rel_file.endswith('.ts') and not rel_file.endswith('.d.ts')
    ]


# ts_library type-checks via `tsc -p <tsconfig>` and emits declarations by
# passing its sources on the command line, so the files it is about to compile
# come from one of those two places.
def sources_to_compile(command_args):
    if '-p' in command_args:
        return tsconfig_sources(command_args[command_args.index('-p') + 1])

    return [
        os.path.abspath(arg) for arg in command_args
        if arg.endswith('.ts') and not arg.endswith('.d.ts')
    ]


@override_utils.override_function(globals())
def main(original_function):
    args = expand_response_files(sys.argv[1:])
    if '--' in args:
        sources = sources_to_compile(args[args.index('--') + 1:])
        # Brave sources first: the chromium_src lookup needs them in place.
        hardlink_brave_sources(sources)
        hardlink_chromium_src_overrides(sources)

    with tsc_timeout_retry.patch_subprocess_with_timeout_retry():
        return original_function()

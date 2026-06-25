#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Install Brave's `dep_type: 'aws'` support into a depot_tools checkout.

This copies `gclient_aws.py` into the target depot_tools and inserts two small,
idempotent edits: an `elif dep_type == 'aws'` branch in gclient.py's dependency
dispatch, and a matching schema branch in gclient_eval.py so DEPS using the new
type validate. Re-running is safe -- each edit is skipped if already present.

Usage:
    python3 apply.py [DEPOT_TOOLS_DIR]

DEPOT_TOOLS_DIR defaults to the checkout's vendor/depot_tools (the gclient that
actually drives `gclient sync`). For the proof-of-concept this is run by hand;
it is shaped to later be called right after depot_tools is checked out in
build/commands/lib/depotTools.js.
"""

import os
import shutil
import sys

_HERE = os.path.dirname(os.path.abspath(__file__))
# tools/package/ -> src/brave -> src/brave/vendor/depot_tools
_DEFAULT_DEPOT_TOOLS = os.path.normpath(
    os.path.join(_HERE, '..', '..', 'vendor', 'depot_tools'))

# gclient.py: a new branch in the dep_type dispatch, inserted before `gcs`.
_GCLIENT_ANCHOR = "            elif dep_type == 'gcs':\n"
_GCLIENT_BRANCH = (
    "            elif dep_type == 'aws':\n"
    "                # Brave: archives hosted on Brave's own (public) buckets.\n"
    "                import gclient_aws\n"
    "                deps_to_add.extend(\n"
    "                    gclient_aws.parse_aws_dep(self, name, dep_value,\n"
    "                                              condition, should_process,\n"
    "                                              use_relative_paths,\n"
    "                                              _should_process))\n")
_GCLIENT_MARKER = "elif dep_type == 'aws':"

# gclient_eval.py: a schema branch in `_GCLIENT_DEPS_SCHEMA`, before `# GCS`.
_EVAL_ANCHOR = "        # GCS content.\n"
_EVAL_BRANCH = (
    "        # AWS content (Brave). Mirrors GCS but unauthenticated, with\n"
    "        # size_bytes optional and dep_type required to disambiguate.\n"
    "        _NodeDictSchema({\n"
    "            'bucket':\n"
    "            str,\n"
    "            'objects': [\n"
    "                _NodeDictSchema({\n"
    "                    'object_name':\n"
    "                    str,\n"
    "                    'sha256sum':\n"
    "                    str,\n"
    "                    schema.Optional('size_bytes'):\n"
    "                    int,\n"
    "                    schema.Optional('output_file'):\n"
    "                    str,\n"
    "                    schema.Optional('condition'):\n"
    "                    str,\n"
    "                })\n"
    "            ],\n"
    "            schema.Optional('condition'):\n"
    "            str,\n"
    "            'dep_type':\n"
    "            'aws',\n"
    "        }),\n")
_EVAL_MARKER = "# AWS content (Brave)."


def _insert_before(path, anchor, branch, marker, log):
    """Insert `branch` before `anchor` in `path`; return True if changed."""
    with open(path, 'r') as f:
        content = f.read()
    if marker in content:
        log('  already patched: %s' % os.path.basename(path))
        return False
    if anchor not in content:
        raise SystemExit('  anchor not found in %s; upstream changed, the '
                         'patch needs updating.' % path)
    with open(path, 'w') as f:
        f.write(content.replace(anchor, branch + anchor, 1))
    log('  patched: %s' % os.path.basename(path))
    return True


def _copy_if_differs(src, dst, log):
    """Copy `src` to `dst` only when content differs; return True if changed."""
    if os.path.exists(dst):
        with open(src, 'rb') as a, open(dst, 'rb') as b:
            if a.read() == b.read():
                log('  already current: %s' % os.path.basename(dst))
                return False
    shutil.copyfile(src, dst)
    log('  copied: %s' % os.path.basename(dst))
    return True


def main(argv):
    # `--quiet` keeps no-op syncs silent: it prints a single line only when
    # something actually changed, and nothing when already applied.
    quiet = '--quiet' in argv or '-q' in argv
    positional = [a for a in argv[1:] if not a.startswith('-')]
    depot_tools = os.path.abspath(positional[0]) if positional \
        else _DEFAULT_DEPOT_TOOLS
    if not os.path.isfile(os.path.join(depot_tools, 'gclient.py')):
        raise SystemExit('not a depot_tools checkout: %s' % depot_tools)

    log = (lambda _msg: None) if quiet else print
    log('Installing aws dep_type support into %s' % depot_tools)
    changed = _copy_if_differs(os.path.join(_HERE, 'gclient_aws.py'),
                               os.path.join(depot_tools, 'gclient_aws.py'), log)
    changed |= _insert_before(os.path.join(depot_tools, 'gclient.py'),
                              _GCLIENT_ANCHOR, _GCLIENT_BRANCH, _GCLIENT_MARKER,
                              log)
    changed |= _insert_before(os.path.join(depot_tools, 'gclient_eval.py'),
                              _EVAL_ANCHOR, _EVAL_BRANCH, _EVAL_MARKER, log)
    if quiet and changed:
        print('Applied Brave gclient patches (aws dep_type) to %s' %
              depot_tools)
    log('Done.')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

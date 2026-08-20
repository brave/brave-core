# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Checks that swiftc arguments can still be extracted from a GN build dir.

gn_swift_args.py depends on things outside this repository: the shape of
//build/toolchain/apple/swiftc.py, the `swift` ninja rule and the generated
ninja files. Those can change under us during a Chromium roll. This verifies
the extraction end to end, so the breakage is a test failure rather than an
editor that quietly stops autocompleting.

    python3 gn_swift_args_test.py --out-dir out/ios_current_link

Requires the Swift targets in that output directory to have been built, since
typechecking resolves the modules they import.
"""

import argparse
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import gn_swift_args


def _fail(message):
    print(f'FAIL: {message}')
    return False


def check_targets_found(targets, out_dir):
    """Every output directory with Swift code should yield some targets."""
    if not targets:
        return _fail(f'no Swift targets found in {out_dir}. Either the ninja '
                     f'files are missing (run `gn gen`) or the `swift` build '
                     f'edge format changed.')
    print(f'ok: found {len(targets)} Swift target(s)')
    return True


def check_arguments_extractable(targets, source_root):
    """Argument extraction must succeed and yield the source files."""
    failures = []
    for target in targets:
        try:
            arguments = gn_swift_args.compiler_arguments(target, source_root)
        except Exception as error:  # pylint: disable=broad-except
            failures.append(f'{target.label}: {error}')
            continue
        if not any(argument.endswith('.swift') for argument in arguments):
            failures.append(f'{target.label}: no .swift files in arguments, '
                            f'the SwiftFileList response file may have moved')
    if failures:
        for failure in failures:
            print(f'FAIL: {failure}')
        return False
    print(f'ok: extracted arguments for all {len(targets)} target(s)')
    return True


def check_typechecks(target, source_root):
    """The extracted arguments must actually typecheck the module.

    This is the check that catches semantic drift: arguments that still look
    plausible but no longer describe how the module is built (a missing search
    path, a dropped interop flag) fail here.
    """
    arguments = gn_swift_args.compiler_arguments(target, source_root)
    process = subprocess.run(['xcrun', 'swiftc', '-typecheck'] + arguments,
                             cwd=target.out_dir,
                             capture_output=True,
                             text=True,
                             check=False)
    if process.returncode:
        return _fail(f'{target.label} does not typecheck with the extracted '
                     f'arguments:\n{process.stderr[-2000:]}')
    print(f'ok: {target.label} typechecks')
    return True


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root',
                        default=os.path.abspath(
                            os.path.join(os.path.dirname(__file__), '..', '..',
                                         '..', '..')),
                        help='path to the Chromium src directory')
    parser.add_argument('--out-dir',
                        required=True,
                        help='GN output directory, e.g. out/ios_sim')
    parser.add_argument('--typecheck-all',
                        action='store_true',
                        help='typecheck every target instead of Brave ones '
                        'only (slow)')
    args = parser.parse_args(argv)

    source_root = os.path.abspath(args.source_root)
    out_dir = os.path.realpath(os.path.join(source_root, args.out_dir))
    targets = gn_swift_args.find_targets(out_dir)

    results = [
        check_targets_found(targets, out_dir),
        check_arguments_extractable(targets, source_root),
    ]

    # Typechecking is the slow part, so by default only Brave's own targets are
    # checked; upstream ones may not have been built in this output directory.
    to_typecheck = [
        target for target in targets
        if args.typecheck_all or target.label.startswith('//brave')
    ]
    if not to_typecheck:
        print('warning: no targets to typecheck')
    for target in to_typecheck:
        results.append(check_typechecks(target, source_root))

    if all(results):
        print('\nall checks passed')
        return 0
    print('\nsome checks failed')
    return 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

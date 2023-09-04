#!/usr/bin/env python
# Copyright (c) 2020 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

"""Wraps bin/helper/java_bytecode_rewriter and expands @FileArgs."""

import argparse
import os
import subprocess
import sys

sys.path.append(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, os.pardir,
                 os.pardir, 'build'))
import action_helpers  # pylint: disable=wrong-import-position

sys.path.append(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, os.pardir,
                 os.pardir, 'build', 'android', 'gyp'))
from util import build_utils  # pylint: disable=no-name-in-module,wrong-import-position


def _AddSwitch(parser, val):
    parser.add_argument(
        val, action='store_const', default='--disabled', const=val)


def main(argv):
    argv = build_utils.ExpandFileArgs(argv[1:])
    parser = argparse.ArgumentParser()
    parser.add_argument('--script', required=True,
                        help='Path to the java binary wrapper script.')
    parser.add_argument('--input-jar', required=True)
    parser.add_argument('--output-jar', required=True)
    parser.add_argument('--direct-classpath-jars', required=True)
    parser.add_argument('--sdk-classpath-jars', required=True)
    parser.add_argument('--extra-classpath-jars', dest='extra_jars',
                        action='append', default=[],
                        help='Extra inputs, passed last to the binary script.')
    parser.add_argument('-v', '--verbose', action='store_true')
    parser.add_argument('--missing-classes-allowlist')
    _AddSwitch(parser, '--is-prebuilt')
    _AddSwitch(parser, '--enable-thread-annotations')
    _AddSwitch(parser, '--enable-check-class-path')
    args = parser.parse_args(argv)

    sdk_jars = action_helpers.parse_gn_list(args.sdk_classpath_jars)
    assert len(sdk_jars) > 0

    direct_jars = action_helpers.parse_gn_list(args.direct_classpath_jars)
    assert len(direct_jars) > 0

    extra_classpath_jars = []
    for a in args.extra_jars:
        extra_classpath_jars.extend(action_helpers.parse_gn_list(a))
    args.missing_classes_allowlist = action_helpers.parse_gn_list(
        args.missing_classes_allowlist)

    if args.verbose:
        verbose = '--verbose'
    else:
        verbose = '--not-verbose'

    cmd = ([
        args.script, args.input_jar, args.output_jar, verbose, args.is_prebuilt,
        args.enable_thread_annotations, args.enable_check_class_path
    ] + [str(len(args.missing_classes_allowlist))] +
             args.missing_classes_allowlist + [str(len(sdk_jars))] + sdk_jars +
            [str(len(direct_jars))] + direct_jars + extra_classpath_jars)
    subprocess.check_call(cmd)


if __name__ == '__main__':
    sys.exit(main(sys.argv))

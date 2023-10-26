#!/usr/bin/env vpython3
# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Copyright (C) 2008 Evan Martin <martine@danga.com>

# pylint: disable=no-member,line-too-long

import os
import sys
import re
import traceback

import git_cl
import git_common

def HasFormatErrors():
    # For more options, see vendor/depot_tools/git_cl.py
    cmd = ['cl', 'format', '--diff']
    diff = git_cl.RunGit(cmd).encode('utf-8')
    if diff:
        # Verify that git cl format generates a diff
        if git_common.is_dirty_git_tree('git cl format'):
            # Skip verification if there are uncommitted changes
            print(diff)
            print('Format errors detected. Run npm format locally to fix.')
            return True
        git_cl.RunGit(['cl', 'format'])
        git_diff = git_common.run('diff').encode('utf-8')
        if git_diff:
            print(git_diff)
            print('Format errors have been auto-fixed. Please review and commit these'
                ' changes if lint was run locally. Otherwise run npm format to fix.')
            return True
    return False

def RunFormatCheck(upstream_branch): # pylint: disable=inconsistent-return-statements
    upstream_commit = git_cl.RunGit(['merge-base', 'HEAD', upstream_branch])
    print('Running git cl/gn format on the diff from %s...' % upstream_commit)
    try:
        if HasFormatErrors():
            return 'Format check failed.'
    except Exception:
        e = traceback.format_exc()
        return 'Error running format check:\n' + e


def main(args):
    """Runs cpplint on the current changelist."""
    # Adapted from git_cl.py CMDlint
    parser = git_cl.OptionParser()
    parser.add_option('--filter', action='append', metavar='-x,+y',
                      help='Comma-separated list of cpplint\'s category-filters')
    parser.add_option('--project_root')
    parser.add_option('--base_branch')
    options, args = parser.parse_args(args)

    # Access to a protected member _XX of a client class
    # pylint: disable=protected-access
    try:
        import cpplint # pylint: disable=import-outside-toplevel,syntax-error
        import cpplint_chromium # pylint: disable=import-outside-toplevel
    except ImportError:
        print('Your depot_tools is missing cpplint.py and/or cpplint_chromium.py.')
        return 1

    # Change the current working directory before calling lint so that it
    # shows the correct base.
    settings = git_cl.settings
    previous_cwd = os.getcwd()
    os.chdir(settings.GetRoot())
    cl = git_cl.Changelist()
    base_branch = options.base_branch

    try:
        print('Running cpplint...')
        files = cl.GetAffectedFiles(
            git_common.get_or_create_merge_base(cl.GetBranch(), base_branch))
        if not files:
            print('Cannot lint an empty CL')
            return 0

        # Process cpplints arguments if any.
        command = args + files
        filters = [
            # Allowed by Google style guide.
            '-runtime/references',
            # Most whitespace issues handled by clang-format.
            '-whitespace/braces',
            '-whitespace/comma',
            '-whitespace/end_of_line',
            '-whitespace/forcolon',
            '-whitespace/indent',
            '-whitespace/line_length',
            '-whitespace/newline',
            '-whitespace/operators',
            '-whitespace/parens',
            '-whitespace/semicolon',
            '-whitespace/tab',
        ]
        if options.filter:
            filters.extend(options.filter)
        command = ['--filter=' + ','.join(filters)] + command
        if options.project_root:
            command = ['--project_root=' +
                options.project_root.replace('\\', '/')] + command
        filenames = cpplint.ParseArguments(command)

        white_regex = re.compile(settings.GetLintRegex())
        black_regex = re.compile(settings.GetLintIgnoreRegex())
        extra_check_functions = [cpplint_chromium.CheckPointerDeclarationWhitespace]
        for filename in filenames:
            if white_regex.match(filename):
                if black_regex.match(filename):
                    print('Ignoring file %s' % filename)
                else:
                    cpplint.ProcessFile(filename, cpplint._cpplint_state.verbose_level,
                                        extra_check_functions)
            else:
                print('Skipping file %s' % filename)

        # Run format checks
        format_output = RunFormatCheck(base_branch or cl.GetUpstreamBranch())
    finally:
        os.chdir(previous_cwd)

    if format_output:
        print(format_output)
        return 1
    if cpplint._cpplint_state.error_count != 0:
        print('cpplint errors found: %d\n' % cpplint._cpplint_state.error_count)
        return 1

    print('lint and format checks succeeded')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

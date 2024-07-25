# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
import re

from pathlib import Path

import override_utils

GIT_BAT = 'git.bat'


def ReplaceGitHubSshWithHttps(value):
    return re.sub(r'^git@(github\.com):(.*?\.git)$', r'https://\1/\2', value)


def IsGitIgnored(local_file_dir, local_filename):
    ignore_info = RunCommand(GIT_BAT,
                             'check-ignore',
                             local_filename,
                             cwd=local_file_dir,
                             raise_on_failure=False)
    return bool(ignore_info)


@override_utils.override_function(globals())
def GetCasedFilePath(_orig_func, filename):
    # Original function is flaky. Replace it with Python's Path().
    return str(Path(filename).resolve())


@override_utils.override_function(globals())
def RunCommand(orig_func, *cmd, **kwargs):
    ret = orig_func(*cmd, **kwargs)
    if ret and 'remote.origin.url' in cmd:
        # Replace SSH-based GitHub URLs with HTTPS-based URLs.
        ret = ReplaceGitHubSshWithHttps(ret)

    return ret


@override_utils.override_function(globals())
def ExtractGitInfo(orig_func, local_filename):
    cased_filename = GetCasedFilePath(local_filename)  # pylint: disable=no-value-for-parameter
    local_file_basename = os.path.basename(cased_filename)
    local_file_dir = os.path.dirname(cased_filename)
    if IsGitIgnored(local_file_dir, local_file_basename):
        return None

    return orig_func(local_filename)


@override_utils.override_function(globals())
def DirectoryIsPartOfPublicGitRepository(orig_func, local_dir):
    if IsGitIgnored(local_dir, '.'):
        return False

    return orig_func(local_dir)


@override_utils.override_function(globals())
def IndexFilesFromRepo(orig_func, local_filename, file_list, output_lines,
                       follow_junctions):
    # Replace 'python3' executalbe with 'py -3', because it's installed with the
    # official Python3 installer. 'python3' is not used on Windows.
    python3_line_idx = 8
    python3_replacement = ('python3 -c', 'py -3 -c')
    if python3_replacement[0] in output_lines[python3_line_idx]:
        output_lines[python3_line_idx] = output_lines[python3_line_idx].replace(
            *python3_replacement)
    else:
        # This function is called multiple times with the same `output_lines`
        # list, so on subsequent calls the replacement should already be in
        # place.
        assert python3_replacement[1] in output_lines[python3_line_idx]

    return orig_func(local_filename, file_list, output_lines, follow_junctions)

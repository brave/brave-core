# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from pathlib import Path

import override_utils


@override_utils.override_function(globals())
def GetCasedFilePath(_orig_func, filename):
    # Original function is flaky. Replace it with Python's Path().
    return str(Path(filename).resolve())


@override_utils.override_function(globals())
def RunCommand(orig_func, *cmd, **kwargs):
    # Git output is actually UTF-8, so handle it properly. Assert is added to
    # ensure this won't fail silently if `git.bat` will be replaced.
    assert cmd[0] in ('cmd', 'git.bat'), cmd[0]
    if cmd[0] == 'git.bat':
        kwargs.setdefault('encoding', 'utf-8')
    return orig_func(*cmd, **kwargs)


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
        assert python3_replacement[1] in output_lines[python3_line_idx]

    return orig_func(local_filename, file_list, output_lines, follow_junctions)

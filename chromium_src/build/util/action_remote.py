# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
import subprocess

import override_utils


# Add brave/chromium_src overrides to the input file list for remote execution.
def AddBraveChromiumSrcInputs(args):
    EXEC_ROOT = ('..', '..', '..')
    INPUT_LIST_PATHS_ARG = '--input_list_paths='
    SUPPORTED_EXTS = ('.mojom', '.pdl', '.py')

    def FindArg(args, arg_to_find):
        for arg_idx, arg in enumerate(args):
            if arg.startswith(arg_to_find):
                return arg[len(arg_to_find):], arg_idx

        return None, None

    def ReadFileLines(filepath):
        with open(filepath, 'r') as f:
            for line in f:
                line = line.strip()
                if line:
                    yield line

    input_list_paths, inputs_arg_idx = FindArg(args, INPUT_LIST_PATHS_ARG)
    if not input_list_paths or not inputs_arg_idx:
        return

    input_list_paths = input_list_paths.split(',')

    did_modify_any_input_list = False
    for input_list_idx, input_list_path in enumerate(input_list_paths):
        input_files = []
        did_modify_input_list = False
        for input_file in ReadFileLines(input_list_path):
            input_files.append(input_file)

            if not input_file.endswith(SUPPORTED_EXTS):
                continue

            assert input_file.startswith('src/'), input_file
            new_input = input_file.replace('src/', 'src/brave/chromium_src/', 1)
            if os.path.exists(os.path.join(*EXEC_ROOT, new_input)):
                input_files.append(new_input)
                did_modify_input_list = True

        if did_modify_input_list:
            input_list_path += '_remote.rsp'
            with open(input_list_path, 'w', newline='\n') as f:
                f.write('\n'.join(input_files))
            input_list_paths[input_list_idx] = input_list_path
            did_modify_any_input_list = True

    if did_modify_any_input_list:
        args[inputs_arg_idx] = INPUT_LIST_PATHS_ARG + ','.join(input_list_paths)


@override_utils.override_function(subprocess)
def run(original_function, args, **kwargs):
    AddBraveChromiumSrcInputs(args)

    return original_function(args, **kwargs)

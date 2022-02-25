#!/usr/bin/env python3

# Copyright (c) 2013 GitHub, Inc.
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Convert pdb to sym for given directories"""

import errno
import glob
import argparse
import os
import re
import sys
import asyncio

from datetime import datetime
from shutil import rmtree, move, copy


async def ProcessBinary(semaphore, options, binary):
    dump_syms = os.path.join(options.build_dir, options.dump_syms_path)

    sym_temp_output = binary + '.sym'
    error = None
    async with semaphore:
        start_time = datetime.utcnow()
        if options.verbose:
            print(f'Generating symbols for {binary}')
        with open(sym_temp_output, 'w', encoding='utf-8') as output:
            process = await asyncio.create_subprocess_exec(
                dump_syms,
                binary,
                stdout=output,
                stderr=asyncio.subprocess.PIPE)
            _, stderr = await process.communicate()

            # pylint: disable=fixme
            # TODO: investigate why dump_syms.exe fails and replace to
            # return False
            if process.returncode != 0:
                decoded_stderr = stderr.decode('utf-8')
                error = f'dump_syms return {process.returncode} for {binary}. '\
                        f'stderr:\n{decoded_stderr}'

        module_line = None
        with open(sym_temp_output, encoding='utf-8') as f:
            MODULE_PATTERN = r'MODULE [^ ]+ [^ ]+ ([0-9A-Fa-f]+) (.*)'
            module_line = re.match(MODULE_PATTERN, f.readline())
            if not module_line:
                return False, f'No module name found for {binary}'

        output_path = os.path.join(options.symbols_dir, module_line.group(2),
                                   module_line.group(1))
        mkdir_p(output_path)

        symbol_file = f'{module_line.group(2)[:-4]}.sym'  # strip .pdb
        move(sym_temp_output, os.path.join(output_path, symbol_file))
        if options.verbose:
            elapsed = datetime.utcnow() - start_time
            print(f'Completed generating symbols for {binary}: elapsed time '
                  f'{elapsed.total_seconds()} seconds')

        # Copy all pdb files to the provided directory to create a standalone
        # native symbols archive later (see create_native_symbols_dist target).
        if options.platform_symbols_dir:
            pdb_output_path = os.path.join(options.platform_symbols_dir,
                                           module_line.group(2),
                                           module_line.group(1))
            mkdir_p(pdb_output_path)
            copy(binary, pdb_output_path)
        return True, error


def mkdir_p(path):
    """Simulates mkdir -p."""
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise


async def GenerateSymbols(options, binaries):
    """Dumps the symbols of binary and places them in the given directory."""
    semaphore = asyncio.Semaphore(os.cpu_count())
    task_list = []
    for binary in binaries:
        task_list.append(
            asyncio.create_task(ProcessBinary(semaphore, options, binary)))
    result_list = await asyncio.gather(*task_list)
    first_warning_printed = False
    for success, error_message in result_list:
        if not success:
            print(error_message, file=sys.stderr)
            return False
        if error_message and not first_warning_printed:
            first_warning_printed = True
            print(
                f'Warning: symbol generation failed for some binaries, '
                f'the first error is:\n{error_message}',
                file=sys.stderr)

    return True


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('directories',
                        nargs='+',
                        help='Directories in which to look for pdbs.')
    parser.add_argument('--build-dir',
                        required=True,
                        help='The build output directory.')
    parser.add_argument('--dump-syms-path',
                        required=True,
                        help='The path to dump_syms.exe')
    parser.add_argument('--symbols-dir',
                        required=True,
                        help='The directory where to write the symbols file.')
    parser.add_argument('--platform-symbols-dir',
                        help='Directory to output pdb files')
    parser.add_argument('--clear',
                        default=False,
                        action='store_true',
                        help='Clear the symbols directory before writing new '
                        'symbols.')
    parser.add_argument('-v',
                        '--verbose',
                        action='store_true',
                        help='Print verbose status output.')

    args = parser.parse_intermixed_args()

    if args.clear:
        try:
            rmtree(args.symbols_dir)
        except:  # pylint: disable=bare-except
            pass

    pdbs = []
    for directory in args.directories:
        pdbs += glob.glob(os.path.join(directory, '*.exe.pdb'))
        pdbs += glob.glob(os.path.join(directory, '*.dll.pdb'))

    result = await GenerateSymbols(args, pdbs)
    sys.exit(0 if result else 1)


if __name__ == '__main__':
    asyncio.run(main())

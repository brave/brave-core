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
import shutil

from datetime import datetime
from shutil import rmtree

BRAVE_ROOT = os.path.abspath(
    os.path.dirname(os.path.dirname(os.path.dirname(__file__))))


async def ProcessBinary(semaphore, options, binary):
    dump_syms = os.path.join(options.build_dir, 'dump_syms.exe')
    sym_temp_output = binary + '.sym'
    async with semaphore:
        start_time = datetime.utcnow()
        if options.verbose:
            print("Generating symbols for {0}".format(binary))
        with open(sym_temp_output, 'w') as output:
            process = await asyncio.create_subprocess_exec(
                dump_syms,
                binary,
                stdout=output,
                stderr=asyncio.subprocess.PIPE)
            _, stderr = await process.communicate()

            # TODO(atuchin): investigate why dump_syms.exe fails
            # and replace to False
            # if process.returncode != 0:
            #     decoded_stderr = stderr.decode('utf-8')
            #     return False, \
            #            f"dump_syms failed for {binary} \n {decoded_stderr}"

        module_line = None
        with open(sym_temp_output) as f:
            MODULE_PATTERN = r"MODULE [^ ]+ [^ ]+ ([0-9A-Fa-f]+) (.*)"
            if module_line == None:
                module_line = re.match(MODULE_PATTERN, f.readline())
        if module_line == None:
            return False, "No module name found for " + binary

        output_path = os.path.join(options.symbols_dir, module_line.group(2),
                                   module_line.group(1))
        mkdir_p(output_path)

        symbol_file = "%s.sym" % module_line.group(2)[:-4]  # strip .pdb
        shutil.move(sym_temp_output, symbol_file)
        if options.verbose:
            elapsed = datetime.utcnow() - start_time
            print(
                "Completed generating symbols for {}: elapsed time {} seconds".
                format(binary, elapsed.total_seconds()))
        return True, None


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
    list = []
    for binary in binaries:
        list.append(
            asyncio.ensure_future(ProcessBinary(semaphore, options, binary)))
    result_list = await asyncio.gather(*list)
    for success, error_message in result_list:
        if not success:
            print(error_message, file=sys.stderr)
            return False

    return True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('directories',
                        nargs='+',
                        help='Directories in which to look for pdbs.')
    parser.add_argument('--build-dir',
                        required=True,
                        help='The build output directory.')
    parser.add_argument('--symbols-dir',
                        required=True,
                        help='The directory where to write the symbols file.')
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

    result = asyncio.run(GenerateSymbols(args, pdbs))

    return 1 if result else 0


if __name__ == '__main__':
    sys.exit(main())

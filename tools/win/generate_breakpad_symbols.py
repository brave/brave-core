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
import queue
import re
import subprocess
import sys
import threading

from datetime import datetime
from shutil import rmtree

CONCURRENT_TASKS=1
BRAVE_ROOT=os.path.abspath(os.path.dirname(os.path.dirname(os.path.dirname(__file__))))


def GetCommandOutput(command):
    """Runs the command list, returning its output.

    Prints the given command (which should be a list of one or more strings),
    then runs it and returns its output (stdout) as a string.

    From chromium_utils.
    """
    with open(os.devnull, 'w') as devnull:
        with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=devnull, bufsize=1) as proc:
            output = proc.communicate()[0]
            return output.decode('utf-8')


def mkdir_p(path):
    """Simulates mkdir -p."""
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise


def GenerateSymbols(options, binaries):
    """Dumps the symbols of binary and places them in the given directory."""

    q = queue.Queue()
    print_lock = threading.Lock()

    def _Worker():
        while True:
            binary = q.get()

            if options.verbose:
                with print_lock:
                    print("Generating symbols for {0}".format(binary), flush=True)
                    thread_start = datetime.utcnow()

            dump_syms = os.path.join(options.build_dir, 'dump_syms.exe')
            syms = GetCommandOutput([dump_syms, binary])
            module_line = re.match(r"MODULE [^ ]+ [^ ]+ ([0-9A-Fa-f]+) (.*)\r\n", syms)
            if module_line is None:
                with print_lock:
                    print("Failed to get symbols for {0}".format(binary), flush=True)
                q.task_done()
                continue

            output_path = os.path.join(options.symbols_dir, module_line.group(2),
                                       module_line.group(1))
            mkdir_p(output_path)
            symbol_file = "%s.sym" % module_line.group(2)[:-4]  # strip .pdb
            with open(os.path.join(output_path, symbol_file), 'w') as f:
                f.write(syms)

            if options.verbose:
                with print_lock:
                    thread_end = datetime.utcnow()
                    elapsed = thread_end - thread_start
                    print("Completed generating symbols for {}: elapsed time {} seconds".format(
                        binary, elapsed.total_seconds()), flush=True)

            q.task_done()

    for binary in binaries:
        q.put(binary)

    for _ in range(options.jobs):
        t = threading.Thread(target=_Worker)
        t.daemon = True
        t.start()

    q.join()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('directories', nargs='+',
                        help='Directories in which to look for pdbs.')
    parser.add_argument('--build-dir', required=True,
                        help='The build output directory.')
    parser.add_argument('--symbols-dir', required=True,
                        help='The directory where to write the symbols file.')
    parser.add_argument('--clear', default=False, action='store_true',
                        help='Clear the symbols directory before writing new '
                            'symbols.')
    parser.add_argument('-j', '--jobs', default=CONCURRENT_TASKS, action='store',
                        type=int, help='Number of parallel tasks to run.')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Print verbose status output.')

    args = parser.parse_intermixed_args()

    if args.clear:
        try:
            rmtree(args.symbols_dir)
        except: # pylint: disable=bare-except
            pass

    pdbs = []
    for directory in args.directories:
        pdbs += glob.glob(os.path.join(directory, '*.exe.pdb'))
        pdbs += glob.glob(os.path.join(directory, '*.dll.pdb'))

    GenerateSymbols(args, pdbs)

    return 0


if __name__ == '__main__':
    sys.exit(main())

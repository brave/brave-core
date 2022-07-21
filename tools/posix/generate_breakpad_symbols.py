#!/usr/bin/env python3

# Copyright (c) 2013 GitHub, Inc.
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A tool to generate symbols for a binary suitable for breakpad.

Currently, the tool only supports Linux, Android, and Mac. Support for other
platforms is planned.
"""

import errno
import argparse
import os
import queue
import re
import shutil
import subprocess
import sys
import threading

sys.path.append(os.path.join(os.path.dirname(__file__),
                             os.pardir, os.pardir, os.pardir,
                             "build"))
import gn_helpers # pylint: disable=wrong-import-position

CONCURRENT_TASKS=4


def GetCommandOutput(command):
    """Runs the command list, returning its output.

    Prints the given command (which should be a list of one or more strings),
    then runs it and returns its output (stdout) as a string.

    From chromium_utils.
    """
    devnull = open(os.devnull, 'w') # pylint: disable=consider-using-with
    proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=devnull) # pylint: disable=consider-using-with
    output = proc.communicate()[0]
    return output.decode('utf-8')


def GetDumpSymsBinary(build_dir=None):
    """Returns the path to the dump_syms binary."""
    DUMP_SYMS = 'dump_syms'
    dump_syms_bin = os.path.join(os.path.expanduser(build_dir), DUMP_SYMS)
    if not os.access(dump_syms_bin, os.X_OK):
        print("Cannot find {0}.".format(DUMP_SYMS))
        sys.exit(1)

    return dump_syms_bin


def FindBundlePart(full_path):
    if full_path.endswith(('.dylib', '.framework', '.app')):
        return os.path.basename(full_path)
    if full_path not in ('', '/'):
        return FindBundlePart(os.path.dirname(full_path))
    return ''


def GetDSYMBundle(options, binary_path):
    """Finds the .dSYM bundle to the binary."""
    if os.path.isabs(binary_path):
        dsym_path = binary_path + '.dSYM'
        if os.path.exists(dsym_path):
            return dsym_path

    filename = FindBundlePart(binary_path)
    search_dirs = [options.build_dir, options.libchromiumcontent_dir]
    if filename.endswith(('.dylib', '.framework', '.app')):
        for directory in search_dirs:
            dsym_path = os.path.join(directory, os.path.splitext(filename)[0]) + '.dSYM'
            if os.path.exists(dsym_path):
                return dsym_path
            dsym_path = os.path.join(directory, filename) + '.dSYM'
            if os.path.exists(dsym_path):
                return dsym_path

    return binary_path


def GetSymbolPath(options, binary_path):
    """Finds the .dbg to the binary."""
    filename = os.path.basename(binary_path)
    dbg_path = os.path.join(options.libchromiumcontent_dir, filename) + '.dbg'
    if os.path.exists(dbg_path):
        return dbg_path

    return binary_path


def Resolve(path, exe_path, loader_path, rpaths):
    """Resolve a dyld path.

    @executable_path is replaced with |exe_path|
    @loader_path is replaced with |loader_path|
    @rpath is replaced with the first path in |rpaths| where the referenced file
        is found
    """
    path = path.replace('@loader_path', loader_path)
    path = path.replace('@executable_path', exe_path)
    if path.find('@rpath') != -1:
        for rpath in rpaths:
            new_path = Resolve(path.replace('@rpath', rpath), exe_path, loader_path, [])
            if os.access(new_path, os.F_OK):
                return new_path
        return ''
    return path


def GetSharedLibraryDependenciesLinux(binary):
    """Return absolute paths to all shared library dependecies of the binary.

    This implementation assumes that we're running on a Linux system."""
    ldd = GetCommandOutput(['ldd', binary])
    lib_re = re.compile(r'\t.* => (.+) \(.*\)$')
    result = []
    for line in ldd.splitlines():
        m = lib_re.match(line)
        if m:
            result.append(m.group(1))
    return result


def GetSharedLibraryDependenciesMac(binary, exe_path):
    """Return absolute paths to all shared library dependecies of the binary.

    This implementation assumes that we're running on a Mac system."""
    loader_path = os.path.dirname(binary)
    otool = GetCommandOutput(['otool', '-l', binary]).splitlines()
    rpaths = []
    for idx, line in enumerate(otool):
        if line.find('cmd LC_RPATH') != -1:
            m = re.match(r' *path (.*) \(offset .*\)$', otool[idx+2])
            rpaths.append(m.group(1))

    otool = GetCommandOutput(['otool', '-L', binary]).splitlines()
    lib_re = re.compile(r'\t(.*) \(compatibility .*\)$')
    deps = []
    for line in otool:
        m = lib_re.match(line)
        if m:
            dep = Resolve(m.group(1), exe_path, loader_path, rpaths)
            if dep:
                deps.append(os.path.normpath(dep))
    return deps


def GetSharedLibraryDependencies(binary, exe_path):
    """Return absolute paths to all shared library dependecies of the binary."""
    deps = []
    if sys.platform.startswith('linux'):
        deps = GetSharedLibraryDependenciesLinux(binary)
    elif sys.platform == 'darwin':
        deps = GetSharedLibraryDependenciesMac(binary, exe_path)
    else:
        print("Platform not supported.")
        sys.exit(1)

    result = []
    for dep in deps:
        if os.access(dep, os.F_OK):
            result.append(dep)
    return result


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

            try:
                if options.verbose:
                    with print_lock:
                        print("Generating symbols for {0}".format(binary))

                if sys.platform == 'darwin':
                    binary = GetDSYMBundle(options, binary)
                elif sys.platform == 'linux2':
                    binary = GetSymbolPath(options, binary)

                syms = GetCommandOutput([GetDumpSymsBinary(options.build_dir), '-r', '-c', binary])
                module_line = re.match("MODULE [^ ]+ [^ ]+ ([0-9A-F]+) (.*)\n", syms)
                output_path = os.path.join(options.symbols_dir, module_line.group(2),
                                           module_line.group(1))
                mkdir_p(output_path)
                symbol_file = "%s.sym" % module_line.group(2)
                f = open(os.path.join(output_path, symbol_file), 'w') # pylint: disable=consider-using-with
                f.write(syms)
                f.close()
            except Exception as inst: # pylint: disable=broad-except
                if options.verbose:
                    with print_lock:
                        print(type(inst))
                        print(inst)
            finally:
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
    parser.add_argument('--build-dir', required=True,
                        help='The build output directory.')
    parser.add_argument('--symbols-dir', required=True,
                        help='The directory where to write the symbols file.')
    parser.add_argument('--libchromiumcontent-dir', required=True,
                        help='The directory where libchromiumcontent is downloaded.')
    parser.add_argument('--binary', required=True,
                        help='The path of the binary to generate symbols for.')
    parser.add_argument('--clear', default=False, action='store_true',
                        help='Clear the symbols directory before writing new symbols.')
    parser.add_argument('-j', '--jobs', default=CONCURRENT_TASKS, action='store',
                        type=int, help='Number of parallel tasks to run.')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Print verbose status output.')

    options = parser.parse_args()

    if options.clear:
        try:
            shutil.rmtree(options.symbols_dir)
        except: # pylint: disable=bare-except
            pass

    parser = gn_helpers.GNValueParser(options.binary)
    binary = parser.ParseList()

    # Build the transitive closure of all dependencies.
    binaries = set(binary)
    q = binary
    exe_path = os.path.dirname(binary[0])
    while q:
        deps = GetSharedLibraryDependencies(q.pop(0), exe_path)
        new_deps = set(deps) - binaries
        binaries |= new_deps
        q.extend(list(new_deps))

    GenerateSymbols(options, binaries)

    return 0


if __name__ == '__main__':
    sys.exit(main())

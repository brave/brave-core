#!/usr/bin/env python3

"""A tool to generate symbols for a binary suitable for breakpad.

Wrapper for components/crash/content/tools/generate_breakpad_symbols.py for
Android which takes the apk or aab and generates symbols for all libs
"""

import argparse
import glob
import multiprocessing
import os
import re
import queue
import shutil
import sys
import subprocess
import threading
import zipfile

CONCURRENT_TASKS = multiprocessing.cpu_count()

def GetDumpSymsBinary(build_dir=None):
    """Returns the path to the dump_syms binary."""
    DUMP_SYMS = 'dump_syms'
    dump_syms_bin = os.path.join(os.path.expanduser(build_dir), DUMP_SYMS)
    if not os.access(dump_syms_bin, os.X_OK):
        print(f'Cannot find {dump_syms_bin}.')
        return None

    return dump_syms_bin

def GetRequiredLibsPaths(args, extension):
    """For the given args.package_name returns the list of path of libs,
    which are required to generate the symbols"""

    libs_in_package = [] #libs
    with zipfile.ZipFile(args.package_path, 'r') as zf:
        if extension == '.aab':
            AAB_BASE_LIB_RE = re.compile('base/lib/[^/]*/\\S*[.]so')
            libs_in_package = \
                [s for s in zf.namelist() if AAB_BASE_LIB_RE.match(s)]
        elif extension == '.apk':
            APK_LIB_RE = re.compile('lib/[^/]*/\\S*[.]so')
            libs_in_package = [s for s in zf.namelist() if APK_LIB_RE.match(s)]

    # Set of libs which were not created by us
    ignored_libs = {
        'libarcore_sdk_c.so', # Augmented reality lib from Android SDK
        'libwg-go.so' # Wireguard lib for Brave VPN
        }

    # Additional ABIs
    additional_abi_dirs = glob.glob(os.path.join(args.build_dir,
                                                 'android_clang_*'))

    # Since cr128 there are two additional dirs:
    #   1) 'android_clang_arm64_with_system_allocator'
    #   2) 'android_clang_arm'
    # None of them contains the libs included into bundle.
    # Since PR brave-core/pull/20849 we supply only one ABI per package,
    # So it looks like additional_abi_dirs can be skipped.
    # TODO(alexeybarabash): https://github.com/brave/brave-browser/issues/40305

    libs_only_names = set() #lib_names
    for lib in libs_in_package:
        # Cut out 'crazy.' if exists
        # see //chrome/android/chrome_public_apk_tmpl.gni
        lib_name = os.path.basename(lib).replace('crazy.', '')
        if not lib_name in ignored_libs:
            libs_only_names.add(lib_name)

    libs_result = []

    for lib_name in libs_only_names:
        lib_path = os.path.join(args.build_dir, 'lib.unstripped', lib_name)
        if os.path.exists(lib_path):
            libs_result.append(lib_path)
        else:
            lib_path = os.path.join(args.build_dir, lib_name)
            if os.path.exists(lib_path):
                libs_result.append(lib_path)
            else:
                # Something is wrong, we met the lib in aab/apk, for which we
                # cannot find lib in the build dir
                print('Seeing lib don\'t know the location for: ' + lib_name)
                return []

        if additional_abi_dirs:
            lib_path = os.path.join(additional_abi_dirs[0], 'lib.unstripped',
                                    lib_name)
            if os.path.exists(lib_path):
                libs_result.append(lib_path)

    # Remove the duplicates, can have in the case when apk/aab contains two abis
    libs_result = list(set(libs_result))

    return libs_result


def InvokeChromiumGenerateSymbols(args, lib_paths):
    """Invokes Chromium's script
    components/crash/content/tools/generate_breakpad_symbols.py for each lib
    of lib_paths."""

    q = queue.Queue()

    print_lock = threading.Lock()

    at_least_one_failed = multiprocessing.Value('b', False)

    chromium_script = os.path.join(args.src_root,
                                   'components/crash/content/tools'
                                   '/generate_breakpad_symbols.py')

    def _Worker():
        while True:
            lib_path = q.get()

            try:
                # Invoke the original Chromium script
                args_to_pass = ['vpython3',
                                chromium_script,
                                '--build-dir=' + args.build_dir,
                                '--symbols-dir=' + args.symbols_dir,
                                '--binary=' + lib_path,
                                '--platform=android',
                                '--verbose'
                               ]

                ret = subprocess.call(args_to_pass)

                if ret != 0:
                    # Lets fail just not to ignore something important
                    at_least_one_failed.value = True

            except Exception as e:
                if args.verbose:
                    with print_lock:
                        print(type(e))
                        print(e)
            finally:
                q.task_done()

    for lib_path in lib_paths:
        q.put(lib_path)

    for _ in range(args.jobs):
        t = threading.Thread(target=_Worker)
        t.daemon = True
        t.start()

    q.join()

    if at_least_one_failed.value:
        return 1

    return 0

def main():
    parser = argparse.ArgumentParser(description='Generates symbols for '
                                     'Android package')
    parser.add_argument('--build-dir', type=str, required=True,
                        help='The build output directory.')
    parser.add_argument('--symbols-dir', type=str, required=True,
                        help='The directory where to write the symbols file.')
    parser.add_argument('--package-path', type=str, required=True,
                        help='The path of the apk or aab package to generate '
                        'symbols for libs from it.')
    parser.add_argument('--src-root', type=str, required=True,
                        help='The path of the root src Chromium\'s folder.')
    parser.add_argument('--clear', action='store_true', required=False,
                        help='Clear the symbols directory before writing new '
                        'symbols.')
    parser.add_argument('--jobs', '--j', default=CONCURRENT_TASKS,
                        required=False, type=int,
                        help='Number of parallel tasks to run.')
    parser.add_argument('--verbose', '--v', required=False, action='store_true',
                        help='Print verbose status output.')

    args = parser.parse_args()

    if not GetDumpSymsBinary(args.build_dir):
        return 1

    _, extension = os.path.splitext(args.package_path)

    if extension not in ('.apk', '.aab'):
        print('Input file is not apk or aab')
        return 1

    libs_result = GetRequiredLibsPaths(args, extension)
    if len(libs_result) == 0:
        return 1

    # Clear the dir
    if args.clear:
        try:
            shutil.rmtree(args.symbols_dir)
        except: # pylint: disable=bare-except
            pass

    InvokeChromiumGenerateSymbols(args, libs_result)

    return 0


if __name__ == '__main__':
    sys.exit(main())

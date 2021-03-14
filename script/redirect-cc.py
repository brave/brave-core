#!/usr/bin/env python
import sys
import subprocess
import os.path
import os

IS_WIN32 = sys.platform == 'win32'


def main():
    args = sys.argv[1:]
    brave_path = replace_cc_arg(args)
    if 'CC_WRAPPER' in os.environ:
        args = [os.environ['CC_WRAPPER']] + args
    cc_retcode = subprocess.call(args)
    # To check the redirected file timestamp, it should be marked as dependency for ninja.
    # Linux/MacOS gcc deps format includes this file properly.
    # Windows msvc deps format does not include it, so we do it manually here.
    if IS_WIN32 and cc_retcode == 0 and brave_path:
        # This is a specially crafted string that ninja will look for to create deps.
        sys.stderr.write('Note: including file: %s\n' % brave_path)
    return cc_retcode


def replace_cc_arg(args):
    # Interested in -c <path>.cc
    try:
        if IS_WIN32:
            index_c = args.index('/c')
        else:
            index_c = args.index('-c')
    except Exception:
        # no -c or /c so just skip
        return

    if 0 == len(args) or index_c == len(args) - 1:
        # Something wrong, we have -c but have no path in the next arg
        # Just then give all to cc as is
        return
    index_path = index_c + 1

    path_cc = args[index_path]

    # Get the absolute *.cc path
    abs_path_cc = os.path.abspath(path_cc)

    # Get this `brave/src/brave/script/redirect-cc.py` script absolute location
    this_py_path = os.path.realpath(__file__)

    # Get the original chromium dir location, triple parent of current
    # redirect-cc.py
    chromium_original_dir = os.path.abspath(os.path.join(this_py_path,
                                                         os.pardir, os.pardir,
                                                         os.pardir))

    if len(chromium_original_dir) >= len(abs_path_cc) + 1:
        # Could not get original chromium src dir
        return

    # Relative path
    rel_path = abs_path_cc[len(chromium_original_dir) + 1:]

    # Filter away out and out_x86
    # Maybe this dir can be queried from env variable instead of hardcoding
    OUT_DIR_NAMES = ['out', 'out_x86']
    rel_path_parts = rel_path.split(os.sep, 4)
    if rel_path_parts[0] in OUT_DIR_NAMES:
        if rel_path_parts[2] == 'gen':
            rel_path = os.path.join(rel_path_parts[3], rel_path_parts[4])
        elif rel_path_parts[3] == 'gen':
            # In addition to the regular gen location above, 64-bit builds may
            # have the directory stucture like out/<BUILD>/android_clang_arm or
            # out/<BUILD>/android_clang_x86 on Android, or
            # out/<BUILD>/clang_x64_v8_arm64 on MacOS
            rel_path = rel_path_parts[4]
        else:
            # Don't even try to substitute path for other auto-generated cc
            return

    # Build possible brave/chromium_src_path
    brave_path = os.path.join(chromium_original_dir, 'brave', 'chromium_src',
                              rel_path)
    if os.path.isfile(brave_path):
        # Okay, we can replace
        args[index_path] = os.path.relpath(brave_path, os.path.abspath('.'))
        return brave_path


if __name__ == '__main__':
    sys.exit(main())

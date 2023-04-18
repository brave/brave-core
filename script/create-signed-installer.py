import argparse
import shutil
import sys
from sign_binaries import sign_binary


def main():
    args = parse_args()
    create_signed_installer(args.mini_installer_exe, args.brave_installer_exe,
                            args.skip_signing)


def parse_args():
    parser = argparse.ArgumentParser(description='Create signed installer')
    parser.add_argument('mini_installer_exe')
    parser.add_argument('brave_installer_exe')
    parser.add_argument('--skip_signing', action='store_true')
    return parser.parse_args()


def create_signed_installer(mini_installer_exe, brave_installer_exe,
                            skip_signing):
    shutil.copyfile(mini_installer_exe, brave_installer_exe)
    if not skip_signing:
        sign_binary(brave_installer_exe)


if __name__ == '__main__':
    sys.exit(main())

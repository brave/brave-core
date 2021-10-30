import argparse
import os
import shutil
import sys
from sign_binaries import sign_binary


def main():
    args = parse_args()
    root_out_dir = os.path.join(
        os.path.dirname(os.path.dirname(os.path.dirname(
            os.path.realpath(__file__)))),
        args.root_out_dir[0])
    create_signed_installer(
        root_out_dir, args.brave_installer_exe[0], args.skip_signing)


def parse_args():
    parser = argparse.ArgumentParser(description='Create signed installer')
    parser.add_argument('--root_out_dir', nargs=1)
    parser.add_argument('--brave_installer_exe', nargs=1)
    parser.add_argument('--skip_signing', action='store_true')
    return parser.parse_args()


def create_signed_installer(root_out_dir, brave_installer_exe, skip_signing, env=None):
    installer_file = os.path.join(root_out_dir, 'brave_installer.exe')
    shutil.copyfile(os.path.join(root_out_dir, 'mini_installer.exe'),
                    installer_file)
    if not skip_signing:
        sign_binary(installer_file)
    # Copy signed installer to version appended name
    installer_file_with_version = os.path.join(root_out_dir,
                                               brave_installer_exe)
    shutil.copyfile(installer_file, installer_file_with_version)


if __name__ == '__main__':
    sys.exit(main())

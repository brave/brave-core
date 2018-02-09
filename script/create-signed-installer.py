import argparse
import os
import shutil
import sys
from sign_binaries import sign_binary

def main():
  args = parse_args()
  root_out_dir = os.path.join(
    os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))),
    args.root_out_dir[0])
  create_signed_installer(root_out_dir)

def parse_args():
  parser = argparse.ArgumentParser(description='Create signed installer')
  parser.add_argument('--root_out_dir',
                      nargs=1)
  return parser.parse_args()


def create_signed_installer(root_out_dir, env=None):
  installer_file = os.path.join(root_out_dir, 'brave_installer.exe')
  shutil.copyfile(os.path.join(root_out_dir, 'mini_installer.exe'), installer_file)
  sign_binary(installer_file)


if __name__ == '__main__':
  sys.exit(main())

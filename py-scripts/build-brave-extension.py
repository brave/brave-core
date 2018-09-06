import argparse
import os
import sys
import shutil
from lib.util import execute_stdout, scoped_cwd

NPM = 'npm'
if sys.platform in ['win32', 'cygwin']:
  NPM += '.cmd'


def main():
  parser = argparse.ArgumentParser(description='Build Brave extension')
  parser.add_argument('--output_dir')
  parser.add_argument('--brave_extension_dir')
  parser.add_argument('--build_dir')
  args = parser.parse_args()

  brave_extension_dir = os.path.abspath(args.brave_extension_dir)
  build_dir = os.path.abspath(args.build_dir)
  output_dir = os.path.abspath(args.output_dir)

  os.chdir(brave_extension_dir)
  build_extension('.', build_dir)
  copy_output(build_dir, output_dir)


def build_extension(dirname, build_dir, env=None):
  if env is None:
    env = os.environ.copy()

  env["TARGET_GEN_DIR"] = os.path.abspath(build_dir)

  args = [NPM, 'run', 'build']
  with scoped_cwd(dirname):
    execute_stdout(args, env)


def copy_output(build_dir, output_dir):
  try:
    shutil.rmtree(output_dir)
    os.rmdir(output_dir)
  except:
    pass
  shutil.copytree(build_dir, output_dir)

if __name__ == '__main__':
  sys.exit(main())

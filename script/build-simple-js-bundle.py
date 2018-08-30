#!/usr/bin/env python

import argparse
import os
import sys
import contextlib
import subprocess
from shutil import copyfile

def is_verbose_mode():
    return 1

def execute_stdout(argv, env=os.environ):
  if is_verbose_mode():
    print ' '.join(argv)
    try:
      subprocess.check_call(argv, env=env)
    except subprocess.CalledProcessError as e:
      print e.output
      raise e
  else:
    execute(argv, env)



@contextlib.contextmanager
def scoped_cwd(path):
  cwd = os.getcwd()
  os.chdir(path)
  try:
    yield
  finally:
    os.chdir(cwd)

NPM = 'npm'
if sys.platform in ['win32', 'cygwin']:
  NPM += '.cmd'

def main():
  args = parse_args()
  build_bundle(args.repo_dir_path)

def parse_args():
  parser = argparse.ArgumentParser(description='Build js bundle')
  parser.add_argument('-d', '--repo_dir_path',
                      help='Dir where to make the bundle')
  return parser.parse_args()


def build_bundle(dir_path, env=None):
  if env is None:
    env = os.environ.copy()

  args = [NPM, 'install']
  with scoped_cwd(dir_path):
    execute_stdout(args, env)

  args = [NPM, 'run', 'build']
  with scoped_cwd(dir_path):
    execute_stdout(args, env)

if __name__ == '__main__':
  sys.exit(main())

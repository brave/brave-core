#!/usr/bin/env python

import argparse
import os
import sys
import subprocess


verbose_mode = False

def enable_verbose_mode():
  print 'Running in verbose mode'
  global verbose_mode
  verbose_mode = True

def is_verbose_mode():
  return verbose_mode

def execute(argv, env=os.environ, cwd=None):
  if is_verbose_mode():
    print ' '.join(argv)
  try:
    output = subprocess.check_output(argv, stderr=subprocess.STDOUT, env=env, cwd=cwd)
    if is_verbose_mode():
      print output
    return output
  except subprocess.CalledProcessError as e:
    print e.output
    raise e

IGNORE_FILES = set(os.path.join(*components) for components in [
  ['src', 'stmr.h'],
  ['src', 'stmr.cc'],
])

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

def main():

  parser = argparse.ArgumentParser(
    description="Run cpplint on Electron's C++ files",
    formatter_class=argparse.RawTextHelpFormatter
  )
  parser.add_argument(
    '-c', '--only-changed',
    action='store_true',
    default=False,
    dest='only_changed',
    help='only run on changed files'
  )
  parser.add_argument(
    '-v', '--verbose',
    action='store_true',
    default=False,
    dest='verbose',
    help='show cpplint output'
  )
  args = parser.parse_args()

  if not os.path.isfile(cpplint_path()):
    print("[INFO] Skipping cpplint, dependencies has not been bootstrapped")
    return

  if args.verbose:
    enable_verbose_mode()

  os.chdir(SOURCE_ROOT)
  files = find_files(['src', 'include'], is_cpp_file)
  files -= IGNORE_FILES
  if args.only_changed:
    files &= find_changed_files()
  call_cpplint(files)


def find_files(roots, test):
  matches = set()
  for root in roots:
    for parent, _, children, in os.walk(root):
      for child in children:
        filename = os.path.join(parent, child)
        if test(filename):
          matches.add(filename)
  return matches


def is_cpp_file(filename):
  return filename.endswith('.cc') or filename.endswith('.h')


def find_changed_files():
  return set(execute(['git', 'diff', '--name-only']).splitlines())


def call_cpplint(files):
  if files:
    cpplint = cpplint_path()
    execute([sys.executable, cpplint] + list(files))


def cpplint_path():
  if "DEPOT_TOOLS" not in os.environ:
    print("[ERROR] Skipping cpplint, please point DEPOT_TOOLS env variable to depot path.")
    sys.exit(-1)
  return os.path.join(os.environ["DEPOT_TOOLS"], 'cpplint.py')


if __name__ == '__main__':
  sys.exit(main())
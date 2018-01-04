
import argparse
import os
import sys
from lib.util import execute_stdout, scoped_cwd

NPM = 'npm'
if sys.platform in ['win32', 'cygwin']:
  NPM += '.cmd'

def main():
  dir_path = os.path.join(os.path.realpath(os.path.dirname(
      os.path.dirname(os.path.realpath(__file__)))),
      'vendor', 'brave-extension')
  os.chdir(dir_path)
  args = parse_args()
  update_node_modules('.', args.verbose)


def parse_args():
  parser = argparse.ArgumentParser(description='Bootstrap this project')
  parser.add_argument('-v', '--verbose',
                      action='store_true',
                      help='Prints the output of the subprocesses')
  return parser.parse_args()


def update_node_modules(dirname, verbose, env=None):
  if env is None:
    env = os.environ.copy()

  args = [NPM, 'install']
  if verbose:
    args += ['--verbose']
  with scoped_cwd(dirname):
    execute_stdout(args, env)

if __name__ == '__main__':
  sys.exit(main())


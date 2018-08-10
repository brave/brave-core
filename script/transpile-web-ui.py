import argparse
import os
import sys
from lib.util import execute_stdout, scoped_cwd

NPM = 'npm'
if sys.platform in ['win32', 'cygwin']:
  NPM += '.cmd'

def main():
  args = parse_args()
  transpile_web_uis(args.production, args.target_gen_dir[0])


def parse_args():
  parser = argparse.ArgumentParser(description='Transpile web-uis')
  parser.add_argument('-p', '--production',
                      action='store_true',
                      help='Uses production config')
  parser.add_argument('--target_gen_dir',
                      nargs=1)
  return parser.parse_args()


def transpile_web_uis(production, target_gen_dir, env=None):
  if env is None:
    env = os.environ.copy()

  if production:
    args = [NPM, 'run', 'web-ui']
  else:
    args = [NPM, 'run', 'web-ui-dev']

  env["TARGET_GEN_DIR"] = os.path.abspath(target_gen_dir)
  dirname = os.path.abspath(os.path.join(__file__, '..', '..'))
  with scoped_cwd(dirname):
    execute_stdout(args, env)

if __name__ == '__main__':
  sys.exit(main())



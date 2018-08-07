import argparse
import os
import sys
from lib.util import execute_stdout, scoped_cwd
from shutil import copyfile

NPM = 'npm'
if sys.platform in ['win32', 'cygwin']:
  NPM += '.cmd'

def main():
  args = parse_args()

  build_brave_webtorrent(args.production, args.target_gen_dir[0])
  copy_output(args.target_gen_dir[0])

def parse_args():
  parser = argparse.ArgumentParser(description='Build brave-webtorrent')
  parser.add_argument('-p', '--production',
                      action='store_true',
                      help='Uses production config')
  parser.add_argument('--target_gen_dir',
                      nargs=1)
  return parser.parse_args()

def build_brave_webtorrent(production, target_gen_dir, env=None):
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

def copy_output(target_gen_dir):
  files = ['brave_webtorrent_background.bundle.js',
           'brave_webtorrent.bundle.js']
  src_dir = os.path.abspath(os.path.join(__file__, '..', '..', '..',
                            target_gen_dir, 'brave'))
  dst_dir = os.path.abspath(os.path.join(__file__, '..', '..', 'browser',
                                         'resources', 'brave_webtorrent'))
  for f in files:
    copyfile(os.path.join(src_dir, f), os.path.join(dst_dir, f))

if __name__ == '__main__':
  sys.exit(main())

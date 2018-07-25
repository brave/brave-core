import argparse
import os
import sys
#from lib.util import execute_stdout, scoped_cwd
import contextlib
import subprocess
from shutil import copyfile

def is_verbose_mode():
    return 1

def execute_stdout(argv, env=os.environ):
  if is_verbose_mode():
    print ' '.join(argv)
    try:
      print 'os.getcwd()== ' + os.getcwd()
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


def main():
  args = parse_args()
  build_bundle(args.repo_dir_path, args.result_src, args.result_dst)

def parse_args():
  parser = argparse.ArgumentParser(description='Build js bundle')
  parser.add_argument('-d', '--repo_dir_path',
                      help='Dir where to make the bundle')
  parser.add_argument('-s', '--result_src',
                      help='Relative dir in repo where to take result bundle from')
  parser.add_argument('-t', '--result_dst',
                      help='Absolute dir where to copy result to')
  return parser.parse_args()


def build_bundle(dir_path, result_src, result_dst, env=None):
  if env is None:
    env = os.environ.copy()

  args = ['yarn', 'install']
  with scoped_cwd(dir_path):
    execute_stdout(args, env)

  args = ['yarn', 'run', 'build']
  with scoped_cwd(dir_path):
    execute_stdout(args, env)

  result_src_file_path = os.path.join(dir_path, result_src);
  result_dest_file_path = os.path.join(result_dst, result_src)
  #os.path.basename(result_src));
  print 'result_src_file_path=', result_src_file_path
  print 'result_dest_file_path=', result_dest_file_path

  try:
    os.stat(os.path.dirname(result_dest_file_path))
  except:
    os.mkdir(os.path.dirname(result_dest_file_path))

  copyfile(result_src_file_path, result_dest_file_path)

if __name__ == '__main__':
  sys.exit(main())

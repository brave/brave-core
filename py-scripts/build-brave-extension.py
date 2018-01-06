
import os
import sys
import shutil
from lib.util import execute_stdout, scoped_cwd

NPM = 'npm'
if sys.platform in ['win32', 'cygwin']:
  NPM += '.cmd'


def main():
  brave_extension_dir = os.path.realpath(os.path.dirname(
      os.path.dirname(os.path.realpath(__file__))))
  antimuon_dir = os.path.dirname(os.path.dirname(brave_extension_dir))
  brave_extension_gen_dir = os.path.join(
      os.path.dirname(antimuon_dir), 'out', 'Release',
          'gen', 'brave_extension')

  os.chdir(brave_extension_dir)
  build_extension('.')
  build_dir_path  = os.path.join(brave_extension_dir, 'build')
  copy_output(build_dir_path, brave_extension_gen_dir)


def build_extension(dirname, env=None):
  if env is None:
    env = os.environ.copy()

  args = [NPM, 'run', 'build']
  with scoped_cwd(dirname):
    execute_stdout(args, env)


def copy_output(build_dir, output_dir):
  try:
    shutil.rmtree(output_dir)
    os.rmdir(output_dir)
  except:
    pass
  try:
    os.mkdir(DIST_DIR)
    copy_app_to_dist()
  except:
    pass
  shutil.copytree('build', output_dir)


if __name__ == '__main__':
  sys.exit(main())


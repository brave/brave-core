
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
  brave_core_dir = os.path.dirname(os.path.dirname(brave_extension_dir))
  brave_core_src_dir = sys.argv[1]
  brave_extension_browser_resources_dir = os.path.join(brave_core_src_dir, 'browser', 'resources', 'brave_extension')

  os.chdir(brave_extension_dir)
  build_extension('.')
  build_dir_path  = os.path.join(brave_extension_dir, 'build')
  copy_output(build_dir_path, brave_extension_browser_resources_dir)

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


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
  antimuon_src_dir = sys.argv[1]
  brave_extension_browser_resources_dir = os.path.join(antimuon_src_dir, 'browser', 'resources', 'brave_extension')

  os.chdir(brave_extension_dir)
  build_extension('.')
  build_dir_path  = os.path.join(brave_extension_dir, 'build')
  copy_output(build_dir_path, brave_extension_browser_resources_dir)

  brave_out_dir = sys.argv[2]
  locales_src_dir_path = brave_extension_browser_resources_dir;
  locales_dest_dir_path = os.path.join(brave_out_dir, 'resources', 'brave_extension');
  copy_locales(locales_src_dir_path, locales_dest_dir_path)

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

def copy_locales(locales_src_dir_path, locales_dest_dir_path):
  try:
    locales_dest_path = os.path.join(locales_dest_dir_path, '_locales')
    shutil.rmtree(locales_dest_path)
  except:
    pass
  shutil.copytree(os.path.join(locales_src_dir_path, '_locales'), locales_dest_path)

if __name__ == '__main__':
  sys.exit(main())

import os
import sys
import shutil

def main():
  brave_extension_dir = os.path.realpath(os.path.dirname(
      os.path.dirname(os.path.realpath(__file__))))
  brave_extension_browser_resources_dir = os.path.join(brave_extension_dir, 'app')
  locales_src_dir_path = brave_extension_browser_resources_dir;
  brave_out_dir = sys.argv[1]
  locales_dest_dir_path = os.path.join(brave_out_dir, 'brave_extension')
  copy_locales(locales_src_dir_path, locales_dest_dir_path)

def copy_locales(locales_src_dir_path, locales_dest_dir_path):
  try:
    locales_dest_path = os.path.join(locales_dest_dir_path, '_locales')
    shutil.rmtree(locales_dest_path)
  except:
    pass
  shutil.copytree(os.path.join(locales_src_dir_path, '_locales'), locales_dest_path)

if __name__ == '__main__':
  sys.exit(main())

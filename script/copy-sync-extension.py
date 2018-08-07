#!/usr/bin/env python

import sys
import os
import os.path
from shutil import copyfile

# Copies //brave/components/brave_sync/extension =>
#        //brave/browser/resources/

def copy_sync_extension():
  files = ['components/brave_sync/extension/background.js',
           'vendor/brave-sync/bundles/bundle.js',
           'vendor/brave-crypto/browser/crypto.js'];

  this_py_path = os.path.realpath(__file__)
  brave_core_dir = os.path.abspath(os.path.join(this_py_path, os.pardir, os.pardir))
  resources_dir = os.path.abspath(os.path.join(brave_core_dir, 'browser', 'resources', 'brave_sync_extension'))

  if not os.path.exists(resources_dir):
    os.makedirs(resources_dir)

  for rel_path in files:
    src_path = os.path.join(brave_core_dir, rel_path)
    dest_path = os.path.join(resources_dir, os.path.basename(rel_path))
    copyfile(src_path, dest_path)

def main():
  copy_sync_extension()

if __name__ == '__main__':
  sys.exit(main())

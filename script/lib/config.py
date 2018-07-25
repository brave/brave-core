#!/usr/bin/env python

import errno
import json
import os
import platform
import re
import sys

PLATFORM = {
  'cygwin': 'win32',
  'darwin': 'darwin',
  'linux2': 'linux',
  'win32': 'win32',
}[sys.platform]

SOURCE_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
CHROMIUM_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
DIST_URL = 'https://brave-brave-binaries.s3.amazonaws.com/releases/'
BRAVE_CORE_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
BRAVE_BROWSER_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))

verbose_mode = False


def dist_dir():
  return os.path.join(output_dir(), 'dist')

def output_dir():
  if get_target_arch() == 'x64':
    return os.path.join(CHROMIUM_ROOT, 'out', 'Release')
  return os.path.join(CHROMIUM_ROOT, 'out_x86', 'Release')

# Use brave-browser/package.json version for canonical version definition
def brave_browser_package():
  pjson = os.path.join(BRAVE_BROWSER_ROOT, 'package.json')
  with open(pjson) as f:
    obj = json.load(f);
    return obj;

def brave_core_package():
  pjson = os.path.join(BRAVE_CORE_ROOT, 'package.json')
  with open(pjson) as f:
    obj = json.load(f);
    return obj;

def product_name():
  return os.environ.get('npm_config_brave_product_name') or brave_core_package()['name'].split('-')[0]


def project_name():
  return os.environ.get('npm_config_brave_project_name') or brave_core_package()['name'].split('-')[0]


def get_chrome_version():
  version = os.environ.get('npm_config_brave_version') or brave_browser_package()['version']
  return version.split('+')[1]

def get_brave_version():
  return 'v' + get_raw_version()

def get_raw_version():
  return os.environ.get('npm_config_brave_version') or brave_browser_package()['version']

def get_platform_key():
  if os.environ.has_key('MAS_BUILD'):
    return 'mas'
  else:
    return PLATFORM


def get_target_arch():
  return os.environ['TARGET_ARCH'] if os.environ.has_key('TARGET_ARCH') else 'x64'


def get_chromedriver_version():
  pattern = "^chromedriver_version = \"([0-9]\.[0-9]+)\""
  build_gn_path = os.path.join(BRAVE_CORE_ROOT, 'BUILD.gn')
  with open(build_gn_path, 'r') as build_gn_file:
    for line in build_gn_file:
      match = re.search(pattern,line)
      if match:
        version = match.group(1)
  return 'v' + version


def get_env_var(name):
  return os.environ.get('BRAVE_' + name) or os.environ.get('npm_config_BRAVE_' + name, '')


def s3_config():
  config = (get_env_var('S3_BUCKET'),
            get_env_var('S3_ACCESS_KEY'),
            get_env_var('S3_SECRET_KEY'))
  message = ('Error: Please set the $BRAVE_S3_BUCKET, '
             '$BRAVE_S3_ACCESS_KEY, and '
             '$BRAVE_S3_SECRET_KEY environment variables')
  assert all(len(c) for c in config), message
  return config


def enable_verbose_mode():
  print 'Running in verbose mode'
  global verbose_mode
  verbose_mode = True


def is_verbose_mode():
  return verbose_mode


def get_zip_name(name, version, suffix=''):
  arch = get_target_arch()
  if arch == 'arm':
    arch += 'v7l'
  zip_name = '{0}-{1}-{2}-{3}'.format(name, version, get_platform_key(), arch)
  if suffix:
    zip_name += '-' + suffix
  return zip_name + '.zip'

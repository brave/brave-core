#!/usr/bin/env python

from __future__ import print_function
import json
import os
import sys

PLATFORM = {
    'cygwin': 'win32',
    'darwin': 'darwin',
    'linux2': 'linux',
    'linux': 'linux',
    'win32': 'win32',
}[sys.platform]

SOURCE_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', '..'))
CHROMIUM_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', '..', '..'))
BRAVE_CORE_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', '..'))

verbose_mode = False


def dist_dir(target_os, target_arch):
    return os.path.join(output_dir(target_os, target_arch), 'dist')


def output_dir(target_os, target_arch):
    target_os_prefix = ''
    if target_os in ['android', 'ios']:
        target_os_prefix = target_os + '_'

    target_arch_suffix = ''
    if target_arch != 'x64':
        target_arch_suffix = '_' + target_arch

    return os.path.join(CHROMIUM_ROOT, 'out', target_os_prefix + 'Release' + target_arch_suffix)


def brave_core_package():
    pjson = os.path.join(BRAVE_CORE_ROOT, 'package.json')
    with open(pjson) as f:
        obj = json.load(f)
        return obj


def product_name():
    return (os.environ.get('npm_config_brave_product_name') or
            brave_core_package()['name'].split('-')[0])


def get_chrome_version():
    version = (os.environ.get('npm_config_brave_version') or
               brave_core_package()['config']['projects']['chrome']['tag'])
    return version


def get_brave_version():
    return 'v' + get_raw_version()


def get_raw_version():
    return (os.environ.get('npm_config_brave_version') or
            brave_core_package()['version'])


def get_platform_key():
    if 'MAS_BUILD' in os.environ:
        return 'mas'
    else:
        return PLATFORM


def get_env_var(name):
    return (os.environ.get('BRAVE_' + name, '') or
            os.environ.get('npm_config_BRAVE_' + name, ''))


def enable_verbose_mode():
    print('Running in verbose mode')
    global verbose_mode
    verbose_mode = True


def is_verbose_mode():
    return verbose_mode

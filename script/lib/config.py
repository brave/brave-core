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
    'linux': 'linux',
    'win32': 'win32',
}[sys.platform]

SOURCE_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', '..'))
CHROMIUM_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', '..', '..'))
BRAVE_CORE_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', '..'))
BRAVE_BROWSER_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
"""
SHALLOW_BRAVE_BROWSER_ROOT assumes the brave-browser directory is in the same
parent directory as brave-core
"""
SHALLOW_BRAVE_BROWSER_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', '..', '..', 'brave-browser'))
verbose_mode = False


def dist_dir():
    return os.path.join(output_dir(), 'dist')


def output_dir():
    if get_target_arch() == 'x64':
        return os.path.join(CHROMIUM_ROOT, 'out', 'Release')
    return os.path.join(CHROMIUM_ROOT, 'out', 'Release_x86')


# Use brave-browser/package.json version for canonical version definition
def brave_browser_package():
    try:
        pjson = os.path.join(BRAVE_BROWSER_ROOT, 'package.json')
        with open(pjson) as f:
            obj = json.load(f)
            return obj
    except IOError:
        # When IOError exception is caught, try SHALLOW_BRAVE_BROWSER_ROOT next
        try:
            """
            SHALLOW_BRAVE_BROWSER_ROOT assumes the brave-browser directory is in the same
            parent directory as brave-core
            """
            pjson = os.path.join(SHALLOW_BRAVE_BROWSER_ROOT, 'package.json')
            with open(pjson) as f:
                obj = json.load(f)
                return obj
        except Exception as e:
            exit("Error: cannot open file package.json: {}".format(e))


def brave_core_package():
    pjson = os.path.join(BRAVE_CORE_ROOT, 'package.json')
    with open(pjson) as f:
        obj = json.load(f)
        return obj


def product_name():
    return (os.environ.get('npm_config_brave_product_name') or
            brave_core_package()['name'].split('-')[0])


def project_name():
    return (os.environ.get('npm_config_brave_project_name') or
            brave_core_package()['name'].split('-')[0])


def get_chrome_version():
    version = (os.environ.get('npm_config_brave_version') or
               brave_browser_package()['config']['projects']['chrome']['tag'])
    return version


def get_brave_version():
    return 'v' + get_raw_version()


def get_raw_version():
    return (os.environ.get('npm_config_brave_version') or
            brave_browser_package()['version'])


def get_platform_key():
    if 'MAS_BUILD' in os.environ:
        return 'mas'
    else:
        return PLATFORM


def get_target_arch():
    return (os.environ['TARGET_ARCH'] if 'TARGET_ARCH' in os.environ
            else 'x64')


def get_env_var(name):
    return (os.environ.get('BRAVE_' + name) or
            os.environ.get('npm_config_BRAVE_' + name, ''))


def enable_verbose_mode():
    print('Running in verbose mode')
    global verbose_mode
    verbose_mode = True


def is_verbose_mode():
    return verbose_mode


def get_zip_name(name, version, suffix=''):
    arch = get_target_arch()
    if arch == 'arm':
        arch += 'v7l'
    zip_name = '{0}-{1}-{2}-{3}'.format(name, version, get_platform_key(),
                                        arch)
    if suffix:
        zip_name += '-' + suffix
    return zip_name + '.zip'

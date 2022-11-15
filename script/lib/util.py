#!/usr/bin/env python
# Copyright (c) 2021 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

from __future__ import print_function

import atexit
import contextlib
import errno
import platform
import re
import shutil
import ssl
import subprocess
import sys
import tarfile
import tempfile
try:
    from urllib.request import urlopen
except ImportError:
    from urllib2 import urlopen
import os
import zipfile

from .config import is_verbose_mode
from .env_util import get_vs_env

BOTO_DIR = os.path.abspath(os.path.join(__file__, '..', '..', '..', 'vendor',
                                        'boto'))


def get_host_arch():
    """Returns the host architecture with a predictable string."""
    host_arch = platform.machine()

    # Convert machine type to format recognized by gyp.
    if re.match(r'i.86', host_arch) or host_arch == 'i86pc':
        host_arch = 'ia32'
    elif host_arch in ['x86_64', 'amd64']:
        host_arch = 'x64'
    elif host_arch.startswith('arm'):
        host_arch = 'arm'

    # platform.machine is based on running kernel. It's possible to use 64-bit
    # kernel with 32-bit userland, e.g. to give linker slightly more memory.
    # Distinguish between different userland bitness by querying
    # the python binary.
    if host_arch == 'x64' and platform.architecture()[0] == '32bit':
        host_arch = 'ia32'

    return host_arch


def tempdir(prefix=''):
    directory = tempfile.mkdtemp(prefix=prefix)
    atexit.register(shutil.rmtree, directory)
    return directory


@contextlib.contextmanager
def scoped_cwd(path):
    cwd = os.getcwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(cwd)


@contextlib.contextmanager
def scoped_env(key, value):
    origin = ''
    if key in os.environ:
        origin = os.environ[key]
    os.environ[key] = value
    try:
        yield
    finally:
        os.environ[key] = origin


def download(text, url, path):
    safe_mkdir(os.path.dirname(path))
    with open(path, 'wb') as local_file:
        if hasattr(ssl, '_create_unverified_context'):
            # pylint: disable=protected-access
            ssl._create_default_https_context = ssl._create_unverified_context

        web_file = urlopen(url)
        file_size = int(web_file.info().getheaders("Content-Length")[0])
        downloaded_size = 0
        block_size = 128

        ci = os.environ.get('CI') == '1'

        while True:
            buf = web_file.read(block_size)
            if not buf:
                break

            downloaded_size += len(buf)
            local_file.write(buf)

            if not ci:
                percent = downloaded_size * 100. / file_size
                status = "\r%s    %10d    [%3.1f%%]" % (
                    text, downloaded_size, percent)
                print(status)

        if ci:
            print("%s done." % (text))
        else:
            print()
    return path


def extract_tarball(tarball_path, member, destination):
    with tarfile.open(tarball_path) as tarball:
        tarball.extract(member, destination)


def get_lzma_exec():
    root_src_dir = os.path.abspath(
        os.path.join(os.path.dirname(__file__), *[os.pardir] * 3))
    if sys.platform == 'win32':
        lzma_exec = os.path.join(root_src_dir, "third_party", "lzma_sdk",
                                 "bin", "win64", "7za.exe")
    elif sys.platform == 'darwin':
        lzma_exec = os.path.join(root_src_dir, "..", "..", "third_party",
                                 "lzma_sdk", "bin", "mac64", "7zz")
    else:
        lzma_exec = '7zr'  # Use system 7zr.
    return lzma_exec


def extract_zip(zip_path, destination):
    if sys.platform == 'darwin':
        # Use unzip command on Mac to keep symbol links in zip file work.
        execute(['unzip', zip_path, '-d', destination])
    else:
        with zipfile.ZipFile(zip_path) as z:
            z.extractall(destination)


def make_zip(zip_file_path, files, dirs):
    safe_unlink(zip_file_path)
    if sys.platform == 'darwin':
        files += dirs
        execute(['zip', '-r', '-y', zip_file_path] + files)
    else:
        zip_file = zipfile.ZipFile(zip_file_path, "w", zipfile.ZIP_DEFLATED,
                                   allowZip64=True)
        for filename in files:
            zip_file.write(filename, filename)
        for dirname in dirs:
            for root, _, filenames in os.walk(dirname):
                for f in filenames:
                    zip_file.write(os.path.join(root, f))
        zip_file.close()


def make_7z(archive_file_path, files, dirs):
    safe_unlink(archive_file_path)
    files += dirs
    execute([get_lzma_exec(), 'a', '-t7z', '-mx7', archive_file_path] + files)


def rm_rf(path):
    try:
        shutil.rmtree(path)
    except OSError:
        pass


def safe_unlink(path):
    try:
        os.unlink(path)
    except OSError as e:
        if e.errno != errno.ENOENT:
            raise


def safe_mkdir(path):
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise


def execute(argv, env=os.environ):  # pylint: disable=dangerous-default-value
    if is_verbose_mode():
        print(' '.join(argv))
    try:
        if sys.version_info.major == 2:
            process = subprocess.Popen(
                argv, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                universal_newlines=True)
        else:
            process = subprocess.Popen(  # pylint: disable=unexpected-keyword-arg
                argv, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                encoding='utf-8', universal_newlines=True)
        stdout, stderr = process.communicate()
        if is_verbose_mode() or process.returncode != 0:
            if sys.version_info.major == 2:
                printable_stdout = stdout
            else:
                # Fix any unsupported characters in print encoder.
                printable_stdout = stdout.encode(  # pylint: disable=no-member
                    sys.stdout.encoding,
                    'backslashreplace').decode(sys.stdout.encoding)
            # Print the output instead of raising it, so that we get pretty
            # output. Most useful erroroutput from typescript / webpack is in
            # stdout and not stderr.
            print(printable_stdout)
            if process.returncode != 0:
                raise RuntimeError('Command \'%s\' failed' % (' '.join(argv)),
                                   stderr)
        return stdout
    except subprocess.CalledProcessError as e:
        print('Error in subprocess:')
        print(' '.join(argv))
        if sys.version_info.major > 2:
            print(e.stderr)  # pylint: disable=no-member
        raise e


def execute_stdout(argv, env=os.environ):  # pylint: disable=dangerous-default-value
    if is_verbose_mode():
        print(' '.join(argv))
        try:
            subprocess.check_call(argv, env=env)
        except subprocess.CalledProcessError as e:
            print(e.output)
            raise e
    else:
        execute(argv, env)


def parse_version(version):
    if version[0] == 'v':
        version = version[1:]

    vs = version.split('+')[0]
    vs = vs.split('.')

    if len(version.split('+')) == 2:
        vs = vs + [version.split('+')[1]]

    return vs


def boto_path_dirs():
    return [
        os.path.join(BOTO_DIR, 'build', 'lib'),
        os.path.join(BOTO_DIR, 'build', 'lib.linux-x86_64-2.7')
    ]


def run_boto_script(access_key, secret_key, script_name, *args):
    env = os.environ.copy()
    env['AWS_ACCESS_KEY_ID'] = access_key
    env['AWS_SECRET_ACCESS_KEY'] = secret_key
    env['PYTHONPATH'] = os.path.pathsep.join(
        [env.get('PYTHONPATH', '')] + boto_path_dirs())

    boto = os.path.join(BOTO_DIR, 'bin', script_name)
    execute([sys.executable, boto] + list(args), env)


def s3put(bucket, access_key, secret_key, prefix, key_prefix, files):
    args = [
        '--bucket', bucket,
        '--prefix', prefix,
        '--key_prefix', key_prefix,
        '--grant', 'public-read'
    ] + files

    run_boto_script(access_key, secret_key, 's3put', *args)


def import_vs_env(target_arch):
    if sys.platform != 'win32':
        return

    if target_arch == 'ia32':
        vs_arch = 'amd64_x86'
    else:
        vs_arch = 'x86_amd64'
    env = get_vs_env('14.0', vs_arch)
    os.environ.update(env)


def get_platform():
    PLATFORM = {
        'cygwin': 'win32',
        'darwin': 'darwin',
        'linux2': 'linux',
        'win32': 'win32',
    }[sys.platform]
    return PLATFORM

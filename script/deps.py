#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script is used to download deps."""

import os
import shutil
import sys
import tarfile
import tempfile
import time
import zipfile

from lib.util import extract_zip

try:
    from urllib2 import HTTPError, URLError, urlopen
except ImportError:  # For Py3 compatibility
    from urllib.error import HTTPError, URLError # pylint: disable=no-name-in-module,import-error
    from urllib.request import urlopen # pylint: disable=no-name-in-module,import-error


def DownloadUrl(url, output_file):
    """Download url into output_file."""
    CHUNK_SIZE = 4096
    TOTAL_DOTS = 10
    num_retries = 3
    retry_wait_s = 5  # Doubled at each retry.

    while True:
        try:
            sys.stdout.write('Downloading %s ' % url)
            sys.stdout.flush()
            response = urlopen(url)
            total_size = int(response.info().get('Content-Length').strip())
            bytes_done = 0
            dots_printed = 0
            while True:
                chunk = response.read(CHUNK_SIZE)
                if not chunk:
                    break
                output_file.write(chunk)
                bytes_done += len(chunk)
                num_dots = TOTAL_DOTS * bytes_done / total_size
                sys.stdout.write('.' * int(num_dots - dots_printed))
                sys.stdout.flush()
                dots_printed = num_dots
            if bytes_done != total_size:
                raise URLError("only got {} of {} bytes".format(
                    bytes_done, total_size))
            print(" Done.")
            return
        except URLError as e:
            sys.stdout.write('\n')
            print(e)
            if num_retries == 0 or isinstance(
                    e, HTTPError) and e.code in [403, 404]:  # pylint: disable=no-member,line-too-long
                raise e
            num_retries -= 1
            print("Retrying in {} s ...".format(retry_wait_s))
            time.sleep(retry_wait_s)
            retry_wait_s *= 2


def EnsureDirExists(path):
    if not os.path.exists(path):
        os.makedirs(path)


def DownloadAndUnpack(url, output_dir, path_prefix=None):
    """Download an archive from url and extract into output_dir. If path_prefix
       is not None, only extract files whose paths within the archive start
       with path_prefix."""
    with tempfile.NamedTemporaryFile(delete=False) as tmp_file:
        try:
            DownloadUrl(url, tmp_file)
            tmp_file.close()
            try:
                os.unlink(output_dir)
            except OSError:
                pass
            shutil.rmtree(output_dir, ignore_errors=True)
            EnsureDirExists(output_dir)
            if url.endswith('.zip'):
                assert path_prefix is None
                extract_zip(tmp_file.name, output_dir)
            else:
                t = tarfile.open(tmp_file.name, mode='r:gz')
                members = None
                if path_prefix is not None:
                    members = [
                        m for m in t.getmembers()
                        if m.name.startswith(path_prefix)
                    ]
                t.extractall(path=output_dir, members=members)
        finally:
            os.unlink(tmp_file.name)

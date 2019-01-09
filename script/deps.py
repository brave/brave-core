#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script is used to download deps."""

import argparse
import os
import sys
import tarfile
import tempfile
import time
import urllib2
import zipfile


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
            response = urllib2.urlopen(url)
            total_size = int(response.info().getheader('Content-Length').strip())
            bytes_done = 0
            dots_printed = 0
            while True:
                chunk = response.read(CHUNK_SIZE)
                if not chunk:
                    break
                output_file.write(chunk)
                bytes_done += len(chunk)
                num_dots = TOTAL_DOTS * bytes_done / total_size
                sys.stdout.write('.' * (num_dots - dots_printed))
                sys.stdout.flush()
                dots_printed = num_dots
            if bytes_done != total_size:
                raise urllib2.URLError("only got %d of %d bytes" % (bytes_done, total_size))
            print ' Done.'
            return
        except urllib2.URLError as e:
            sys.stdout.write('\n')
            print e
            if num_retries == 0 or isinstance(e, urllib2.HTTPError) and e.code == 404:
                raise e
            num_retries -= 1
            print 'Retrying in %d s ...' % retry_wait_s
            time.sleep(retry_wait_s)
            retry_wait_s *= 2


def EnsureDirExists(path):
    if not os.path.exists(path):
        os.makedirs(path)


def DownloadAndUnpack(url, output_dir, path_prefix=None):
    """Download an archive from url and extract into output_dir. If path_prefix is not
        None, only extract files whose paths within the archive start with path_prefix."""
    with tempfile.TemporaryFile() as f:
        DownloadUrl(url, f)
        f.seek(0)
        EnsureDirExists(output_dir)
        if url.endswith('.zip'):
            assert path_prefix is None
            zipfile.ZipFile(f).extractall(path=output_dir)
        else:
            t = tarfile.open(mode='r:gz', fileobj=f)
            members = None
            if path_prefix is not None:
                members = [m for m in t.getmembers() if m.name.startswith(path_prefix)]
            t.extractall(path=output_dir, members=members)


if __name__ == '__main__':
    sys.exit(main())

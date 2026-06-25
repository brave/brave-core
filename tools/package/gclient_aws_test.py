#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Proof-of-concept test for Brave's gclient `dep_type: 'aws'` support.

Runs apply.py against the checkout's depot_tools (idempotent), then checks two
things with no external network and no `gclient sync`:

  1. gclient_eval accepts a DEPS file declaring `dep_type: 'aws'` (schema patch).
  2. install_object downloads, verifies, extracts and writes sidecars, is a
     no-op on a second run, and rejects a sha256 mismatch (download core).

The download is served from a throwaway http.server on localhost, so the same
code path that hits brave-build-deps-public.s3.brave.com is exercised offline.

Usage:
    vpython3 gclient_aws_test.py   (or python3)
"""

import functools
import hashlib
import http.server
import io
import os
import sys
import tarfile
import tempfile
import threading

_HERE = os.path.dirname(os.path.abspath(__file__))
_DEPOT_TOOLS = os.path.normpath(
    os.path.join(_HERE, '..', '..', 'vendor', 'depot_tools'))

import apply  # noqa: E402  (local module, same dir)


def _serve(directory):
    """Start a localhost HTTP server for `directory`; return (base_url, stop)."""
    handler = functools.partial(http.server.SimpleHTTPRequestHandler,
                                 directory=directory)
    httpd = http.server.ThreadingHTTPServer(('127.0.0.1', 0), handler)
    thread = threading.Thread(target=httpd.serve_forever, daemon=True)
    thread.start()
    host, port = httpd.server_address
    return 'http://%s:%d' % (host, port), httpd.shutdown


def _make_tar_gz(path, payload_name='hello.txt', payload=b'hello aws\n'):
    """Write a .tar.gz containing one file; return its sha256 hex digest."""
    with tarfile.open(path, 'w:gz') as tar:
        info = tarfile.TarInfo(payload_name)
        info.size = len(payload)
        tar.addfile(info, io.BytesIO(payload))
    digest = hashlib.sha256()
    with open(path, 'rb') as f:
        digest.update(f.read())
    return digest.hexdigest()


def test_schema(gclient_eval):
    deps = '''
deps = {
  "src/third_party/example": {
    "bucket": "brave-build-deps-public.s3.brave.com",
    "objects": [
      {
        "object_name": "example/thing-123.tar.xz",
        "sha256sum": "deadbeef",
      },
    ],
    "dep_type": "aws",
  },
}
'''
    result = gclient_eval.Parse(deps, 'DEPS')
    dep = result['deps']['src/third_party/example']
    assert dep['dep_type'] == 'aws', dep
    assert dep['objects'][0]['object_name'] == 'example/thing-123.tar.xz'
    print('PASS test_schema: gclient_eval accepts dep_type aws')


def test_install_object(gclient_aws):
    with tempfile.TemporaryDirectory() as tmp:
        bucket_dir = os.path.join(tmp, 'bucket')
        os.makedirs(bucket_dir)
        object_name = 'toolchain/pkg-1.tar.gz'
        os.makedirs(os.path.join(bucket_dir, 'toolchain'))
        sha = _make_tar_gz(os.path.join(bucket_dir, object_name))

        base_url, stop = _serve(bucket_dir)
        try:
            url = gclient_aws.build_url(base_url, object_name)
            dest = os.path.join(tmp, 'out')

            # First install: downloads, verifies, extracts, writes sidecars.
            assert gclient_aws.install_object(url, sha, None, dest,
                                              object_name) is True
            assert os.path.exists(os.path.join(dest, 'hello.txt'))
            prefix = object_name.replace('/', '_').replace('.', '_')
            assert os.path.exists(os.path.join(dest, '.%s_hash' % prefix))
            assert os.path.exists(
                os.path.join(dest, '.%s_content_names' % prefix))
            print('PASS test_install_object: download + extract + sidecars')

            # Second install: hash already recorded -> no-op.
            assert gclient_aws.install_object(url, sha, None, dest,
                                              object_name) is False
            print('PASS test_install_object: re-run is a no-op')

            # Wrong hash must be rejected.
            try:
                gclient_aws.install_object(url, 'badf00d', None,
                                           os.path.join(tmp, 'out2'),
                                           object_name)
            except Exception as e:  # noqa: BLE001
                assert 'sha256 mismatch' in str(e), e
                print('PASS test_install_object: sha256 mismatch rejected')
            else:
                raise AssertionError('expected sha256 mismatch to raise')
        finally:
            stop()


def main():
    print('Applying aws dep_type support to %s' % _DEPOT_TOOLS)
    apply.main(['apply.py', _DEPOT_TOOLS])

    sys.path.insert(0, _DEPOT_TOOLS)
    import gclient_aws
    import gclient_eval

    test_schema(gclient_eval)
    test_install_object(gclient_aws)
    print('\nAll PoC checks passed.')
    return 0


if __name__ == '__main__':
    sys.exit(main())

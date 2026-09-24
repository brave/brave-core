#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `upload`.

The module's only side effects are `aws` CLI invocations funnelled through
`_check_output`, so every test that touches AWS patches `subprocess.run` with a
dispatcher (`_FakeAws`) that recognises the `kms sign`, `kms verify`, and `s3api
put-object` calls, records the exact argv, and returns canned JSON. The command
strings are then asserted against, which is where the behaviour that matters
lives: the `If-None-Match: *` immutability guard, the server-side checksum, and
the digest-based KMS signing that must mirror `signing.createSignatureKms`.

Layers:

* `Sha256FileTest` runs the real (module-level) hasher over real on-disk files.
* `SummariseTest` checks the public summary carries the location/integrity
  fields and never the KMS signature envelope.
* `KmsVerifyTest` covers the module-level download-side verify helper.
* `SignTest` covers `S3Uploader.sign` argv and the base64-digest encoding.
* `PutObjectTest` covers `S3Uploader._put_object` — the conditional-write and
  checksum flags plus the version-id/etag parsing.
* `UploadTest` drives `S3Uploader.upload` end to end (sign-before-put ordering,
  key/prefix/url resolution, flag propagation, error paths).
* `MainTest` covers the CLI dispatch, `S3Uploader` construction, and flag
  mapping.
"""

from __future__ import annotations

import base64
import contextlib
import dataclasses
import hashlib
import io
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))

import upload as m


class _FakeAws:
    """Dispatcher standing in for `subprocess.run`, faking the `aws` CLI.

    Records every command it is handed (as a list) on `self.commands`, and
    returns a `CompletedProcess` whose stdout is the JSON the matching real
    `aws` subcommand would emit. Unknown commands raise, so a test can never
    accidentally pass against an unexercised code path.
    """

    def __init__(self,
                 signature: str = 'ZmFrZS1zaWc=',
                 version_id: str | None = 'v-1',
                 etag: str | None = '"deadbeef"'):
        self.commands: list[list[str]] = []
        self.envs: list[dict[str, str] | None] = []
        self._signature = signature
        self._version_id = version_id
        self._etag = etag

    def __call__(self, command, **kwargs) -> subprocess.CompletedProcess:
        command = list(command)
        self.commands.append(command)
        self.envs.append(kwargs.get('env'))
        return subprocess.CompletedProcess(command,
                                           0,
                                           stdout=self._stdout(command))

    def _stdout(self, command: list[str]) -> str:
        if 'sts' in command and 'assume-role' in command:
            return json.dumps({
                'Credentials': {
                    'AccessKeyId': 'AKIA_TEMP',
                    'SecretAccessKey': 'temp-secret',
                    'SessionToken': 'temp-token',
                }
            })
        if 'kms' in command and 'sign' in command:
            algorithm = command[command.index('--signing-algorithm') + 1]
            return json.dumps({
                'KeyId': 'arn:aws:kms:...:key/abc',
                'Signature': self._signature,
                'SigningAlgorithm': algorithm,
            })
        if 'kms' in command and 'verify' in command:
            return json.dumps({'SignatureValid': True})
        if 'put-object' in command:
            body: dict[str, object] = {}
            if self._version_id is not None:
                body['VersionId'] = self._version_id
            if self._etag is not None:
                body['ETag'] = self._etag
            return json.dumps(body)
        raise AssertionError(f'unexpected aws command: {command}')

    def command_with(self, needle: str) -> list[str]:
        """Return the single recorded command containing *needle*."""
        matches = [c for c in self.commands if needle in c]
        assert len(
            matches) == 1, f'{needle}: expected 1 command, got {matches}'
        return matches[0]

    def env_for(self, needle: str) -> dict[str, str] | None:
        """Return the env the single command containing *needle* ran with."""
        command = self.command_with(needle)
        return self.envs[self.commands.index(command)]


def _flag(command: list[str], flag: str) -> str:
    """Return the value following *flag* in *command*."""
    return command[command.index(flag) + 1]


@contextlib.contextmanager
def _tempfile(data: bytes = b'toolchain-bytes'):
    """Yield a real file on disk holding *data*."""
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / 'linux-x64-rust-toolchain-abc-1.tar.xz'
        path.write_bytes(data)
        yield path


class Sha256FileTest(unittest.TestCase):

    def test_matches_hashlib(self):
        data = b'the quick brown fox' * 1000
        with _tempfile(data) as path:
            self.assertEqual(m.sha256_file(path),
                             hashlib.sha256(data).hexdigest())

    def test_empty_file(self):
        with _tempfile(b'') as path:
            self.assertEqual(m.sha256_file(path),
                             hashlib.sha256(b'').hexdigest())

    def test_large_file(self):
        # A multi-MiB file exercises the memory-mapped path beyond a page.
        data = b'x' * (4 * 1024 * 1024 + 7)
        with _tempfile(data) as path:
            self.assertEqual(m.sha256_file(path),
                             hashlib.sha256(data).hexdigest())


class SummariseTest(unittest.TestCase):
    """`summarise` emits the public envelope and never the signing material."""

    @staticmethod
    def _result(**overrides):
        base = dict(bucket='brave-build-deps-public',
                    key='nodejs/node_modules.tar.gz',
                    url='https://brave-build-deps-public.s3.brave.com/'
                    'nodejs/node_modules.tar.gz',
                    sha256='abc123',
                    size_bytes=4574694,
                    version_id='v-1',
                    etag='"e-tag"',
                    signature=m.ArtifactSignature(
                        key_id='arn:aws:kms:us-west-2:123456789012:key/abcd',
                        signature='c2lnbmF0dXJlLWJ5dGVz'))
        base.update(overrides)
        return m.UploadResult(**base)

    def test_includes_public_fields(self):
        summary = self._result()
        rendered = m.summarise(summary)
        for expected in (summary.bucket, summary.key, summary.url,
                         summary.sha256, str(summary.size_bytes),
                         summary.version_id, summary.etag):
            self.assertIn(expected, rendered)

    def test_omits_signature_envelope(self):
        rendered = m.summarise(self._result())
        self.assertNotIn('c2lnbmF0dXJlLWJ5dGVz', rendered)
        self.assertNotIn('123456789012', rendered)
        self.assertNotIn('key/abcd', rendered)
        self.assertNotIn('signature', rendered.lower())

    def test_skips_none_fields(self):
        rendered = m.summarise(self._result(version_id=None, etag=None))
        self.assertNotIn('version_id', rendered)
        self.assertNotIn('etag', rendered)
        # A required field is still rendered.
        self.assertIn('sha256', rendered)


class SignTest(unittest.TestCase):

    def test_signs_digest_as_base64_message(self):
        fake = _FakeAws(signature='c2lnMQ==')
        sha256_hex = hashlib.sha256(b'abc').hexdigest()
        with mock.patch.object(m.subprocess, 'run', fake):
            sig = m.S3Uploader('bucket').sign(sha256_hex)

        command = fake.command_with('sign')
        # Message must be the base64 of the raw 32-byte digest, not the hex.
        expected_message = base64.b64encode(bytes.fromhex(sha256_hex)).decode()
        self.assertEqual(_flag(command, '--message'), expected_message)
        self.assertEqual(_flag(command, '--message-type'), 'DIGEST')
        self.assertEqual(_flag(command, '--key-id'), m.DEFAULT_KMS_KEY)
        self.assertEqual(_flag(command, '--signing-algorithm'),
                         m.KMS_ALGORITHM)
        self.assertEqual(_flag(command, '--region'), m.DEFAULT_REGION)

        self.assertEqual(sig.signature, 'c2lnMQ==')
        self.assertEqual(sig.key_id, m.DEFAULT_KMS_KEY)

    def test_signing_algorithm_is_fixed(self):
        # The algorithm is not configurable: it is always KMS_ALGORITHM, whose
        # SHA-256 hash matches the digest produced by sha256_file.
        fake = _FakeAws()
        sha256_hex = hashlib.sha256(b'abc').hexdigest()
        with mock.patch.object(m.subprocess, 'run', fake):
            m.S3Uploader('bucket').sign(sha256_hex)
        command = fake.command_with('sign')
        self.assertEqual(_flag(command, '--signing-algorithm'),
                         'RSASSA_PSS_SHA_256')

    def test_honours_uploader_config(self):
        fake = _FakeAws()
        sha256_hex = hashlib.sha256(b'abc').hexdigest()
        uploader = m.S3Uploader('bucket',
                                region='eu-west-1',
                                kms_key='alias/gpg/other')
        with mock.patch.object(m.subprocess, 'run', fake):
            sig = uploader.sign(sha256_hex)
        command = fake.command_with('sign')
        self.assertEqual(_flag(command, '--key-id'), 'alias/gpg/other')
        self.assertEqual(_flag(command, '--region'), 'eu-west-1')
        self.assertEqual(sig.key_id, 'alias/gpg/other')

    def test_no_role_uses_ambient_credentials(self):
        # Without a sign_role_arn, sign() must not assume a role and must run
        # the kms sign with the ambient env (env=None).
        fake = _FakeAws()
        sha256_hex = hashlib.sha256(b'abc').hexdigest()
        with mock.patch.object(m.subprocess, 'run', fake):
            m.S3Uploader('bucket').sign(sha256_hex)
        self.assertTrue(all('assume-role' not in c for c in fake.commands))
        self.assertIsNone(fake.env_for('sign'))

    def test_assumes_role_when_configured(self):
        # With a sign_role_arn, sign() assumes it and runs kms sign under the
        # returned temporary credentials.
        fake = _FakeAws()
        sha256_hex = hashlib.sha256(b'abc').hexdigest()
        arn = 'arn:aws:iam::123456789012:role/signing-role-x-production'
        with mock.patch.object(m.subprocess, 'run', fake):
            m.S3Uploader('bucket', sign_role_arn=arn).sign(sha256_hex)
        self.assertEqual(_flag(fake.command_with('assume-role'), '--role-arn'),
                         arn)
        env = fake.env_for('sign')
        self.assertEqual(env['AWS_ACCESS_KEY_ID'], 'AKIA_TEMP')
        self.assertEqual(env['AWS_SECRET_ACCESS_KEY'], 'temp-secret')
        self.assertEqual(env['AWS_SESSION_TOKEN'], 'temp-token')

    def test_rejects_non_digest_length(self):
        # A non-SHA-256 hex string must never be signed as if it were a digest.
        with mock.patch.object(m.subprocess, 'run') as run:
            with self.assertRaises(ValueError):
                m.S3Uploader('bucket').sign('abcd')
            run.assert_not_called()

    def test_propagates_kms_failure(self):
        sha256_hex = hashlib.sha256(b'abc').hexdigest()
        err = subprocess.CalledProcessError(255, 'aws')
        with mock.patch.object(m.subprocess, 'run', side_effect=err):
            with self.assertRaises(subprocess.CalledProcessError):
                m.S3Uploader('bucket').sign(sha256_hex)


class KmsVerifyTest(unittest.TestCase):

    def test_builds_verify_command(self):
        fake = _FakeAws()
        sha256_hex = hashlib.sha256(b'abc').hexdigest()
        sig = m.ArtifactSignature(key_id='alias/gpg/k', signature='c2ln')
        with mock.patch.object(m.subprocess, 'run', fake):
            m.kms_verify(sha256_hex, sig)
        command = fake.command_with('verify')
        expected_message = base64.b64encode(bytes.fromhex(sha256_hex)).decode()
        self.assertEqual(_flag(command, '--message'), expected_message)
        self.assertEqual(_flag(command, '--signature'), 'c2ln')
        self.assertEqual(_flag(command, '--key-id'), 'alias/gpg/k')
        self.assertEqual(_flag(command, '--signing-algorithm'),
                         'RSASSA_PSS_SHA_256')

    def test_raises_on_bad_signature(self):
        sig = m.ArtifactSignature('k', 'c2ln')
        err = subprocess.CalledProcessError(1, 'aws')
        with mock.patch.object(m.subprocess, 'run', side_effect=err):
            with self.assertRaises(subprocess.CalledProcessError):
                m.kms_verify(hashlib.sha256(b'abc').hexdigest(), sig)


class PutObjectTest(unittest.TestCase):

    def _put(self, fake, immutable):
        with _tempfile() as path:
            with mock.patch.object(m.subprocess, 'run', fake):
                return m.S3Uploader('bucket')._put_object(path,
                                                          'prefix/obj',
                                                          immutable=immutable)

    def test_immutable_adds_if_none_match(self):
        fake = _FakeAws()
        self._put(fake, immutable=True)
        command = fake.command_with('put-object')
        self.assertEqual(_flag(command, '--if-none-match'), '*')
        # Server-side integrity is always requested.
        self.assertEqual(_flag(command, '--checksum-algorithm'), 'SHA256')

    def test_mutable_omits_if_none_match(self):
        fake = _FakeAws()
        self._put(fake, immutable=False)
        command = fake.command_with('put-object')
        self.assertNotIn('--if-none-match', command)
        self.assertEqual(_flag(command, '--checksum-algorithm'), 'SHA256')

    def test_returns_version_and_etag(self):
        fake = _FakeAws(version_id='v-42', etag='"abc"')
        self.assertEqual(self._put(fake, immutable=True), ('v-42', '"abc"'))

    def test_missing_version_and_etag_are_none(self):
        fake = _FakeAws(version_id=None, etag=None)
        self.assertEqual(self._put(fake, immutable=True), (None, None))


class UploadTest(unittest.TestCase):

    def test_end_to_end(self):
        data = b'archive-payload'
        fake = _FakeAws(signature='c2lnQVo=', version_id='v-9', etag='"e"')
        with _tempfile(data) as path:
            with mock.patch.object(m.subprocess, 'run', fake):
                result = m.S3Uploader('my-bucket').upload(path)

        self.assertEqual(result.bucket, 'my-bucket')
        self.assertEqual(result.key, path.name)
        self.assertEqual(result.url,
                         f'https://my-bucket.s3.brave.com/{path.name}')
        self.assertEqual(result.sha256, hashlib.sha256(data).hexdigest())
        self.assertEqual(result.size_bytes, len(data))
        self.assertEqual(result.version_id, 'v-9')
        self.assertEqual(result.etag, '"e"')
        self.assertEqual(result.signature.signature, 'c2lnQVo=')
        self.assertEqual(_flag(fake.command_with('put-object'), '--bucket'),
                         'my-bucket')
        # Default is a write-once upload.
        self.assertEqual(
            _flag(fake.command_with('put-object'), '--if-none-match'), '*')

    def test_signs_the_uploaded_bytes_digest(self):
        data = b'payload-to-sign'
        fake = _FakeAws()
        with _tempfile(data) as path:
            with mock.patch.object(m.subprocess, 'run', fake):
                m.S3Uploader('b').upload(path)
        signed = _flag(fake.command_with('sign'), '--message')
        expected = base64.b64encode(hashlib.sha256(data).digest()).decode()
        self.assertEqual(signed, expected)

    def test_key_defaults_to_filename(self):
        fake = _FakeAws()
        with _tempfile() as path:
            with mock.patch.object(m.subprocess, 'run', fake):
                result = m.S3Uploader('b').upload(path)
        self.assertEqual(result.key, path.name)

    def test_prefix_composes_key(self):
        fake = _FakeAws()
        with _tempfile() as path:
            with mock.patch.object(m.subprocess, 'run', fake):
                # Trailing slash on the prefix must not double up.
                result = m.S3Uploader('b').upload(path, prefix='some/dir/')
        self.assertEqual(result.key, f'some/dir/{path.name}')
        self.assertEqual(_flag(fake.command_with('put-object'), '--key'),
                         f'some/dir/{path.name}')

    def test_explicit_key_overrides_prefix(self):
        fake = _FakeAws()
        with _tempfile() as path:
            with mock.patch.object(m.subprocess, 'run', fake):
                result = m.S3Uploader('b').upload(path,
                                                  key='custom/name.bin',
                                                  prefix='ignored')
        self.assertEqual(result.key, 'custom/name.bin')
        self.assertEqual(_flag(fake.command_with('put-object'), '--key'),
                         'custom/name.bin')

    def test_url_derived_from_bucket(self):
        fake = _FakeAws()
        with _tempfile() as path:
            with mock.patch.object(m.subprocess, 'run', fake):
                result = m.S3Uploader('some-bucket').upload(path, prefix='p')
        self.assertEqual(result.url,
                         f'https://some-bucket.s3.brave.com/p/{path.name}')

    def test_mutable_upload(self):
        fake = _FakeAws()
        with _tempfile() as path:
            with mock.patch.object(m.subprocess, 'run', fake):
                m.S3Uploader('b').upload(path, immutable=False)
        self.assertNotIn('--if-none-match', fake.command_with('put-object'))

    def test_no_sign_skips_kms(self):
        fake = _FakeAws()
        with _tempfile() as path:
            with mock.patch.object(m.subprocess, 'run', fake):
                result = m.S3Uploader('b').upload(path, sign=False)
        self.assertIsNone(result.signature)
        self.assertTrue(all('kms' not in c for c in fake.commands))

    def test_put_object_keeps_ambient_creds_under_signing_role(self):
        # The signing role has no PutObject; only the sign call may run under
        # it, so put-object must keep the ambient credentials (env=None).
        fake = _FakeAws()
        arn = 'arn:aws:iam::123456789012:role/signing-role-x-production'
        with _tempfile() as path:
            with mock.patch.object(m.subprocess, 'run', fake):
                m.S3Uploader('b', sign_role_arn=arn).upload(path)
        self.assertIsNotNone(fake.env_for('sign'))
        self.assertIsNone(fake.env_for('put-object'))

    def test_missing_file_raises_before_any_aws_call(self):
        with mock.patch.object(m.subprocess, 'run') as run:
            with self.assertRaises(FileNotFoundError):
                m.S3Uploader('b').upload(Path('/does/not/exist.bin'))
            run.assert_not_called()

    def test_sign_failure_aborts_before_put(self):
        # A KMS failure must not leave an unsigned object behind.
        def run(command, **_kwargs):
            command = list(command)
            if 'sign' in command:
                raise subprocess.CalledProcessError(255, 'aws')
            raise AssertionError(
                'put-object must not run after a sign failure')

        with _tempfile() as path:
            with mock.patch.object(m.subprocess, 'run', side_effect=run):
                with self.assertRaises(subprocess.CalledProcessError):
                    m.S3Uploader('b').upload(path)

    def test_precondition_failure_propagates(self):
        # An immutable re-upload (existing key) surfaces as CalledProcessError.
        def run(command, **_kwargs):
            command = list(command)
            if 'sign' in command:
                return subprocess.CompletedProcess(command,
                                                   0,
                                                   stdout=json.dumps({
                                                       'Signature': 's',
                                                       'SigningAlgorithm': 'a'
                                                   }))
            raise subprocess.CalledProcessError(1,
                                                'aws',
                                                stderr='PreconditionFailed')

        with _tempfile() as path:
            with mock.patch.object(m.subprocess, 'run', side_effect=run):
                with self.assertRaises(subprocess.CalledProcessError):
                    m.S3Uploader('b').upload(path)


class MainTest(unittest.TestCase):

    def _run(self, argv):
        out = io.StringIO()
        with mock.patch.object(sys, 'argv', ['upload.py', *argv]):
            with mock.patch.object(m, 'S3Uploader') as cls:
                cls.return_value.upload.return_value = m.UploadResult(
                    bucket='b',
                    key='p/o',
                    url='https://h/p/o',
                    sha256='d',
                    size_bytes=1,
                    version_id='v',
                    etag='"e"',
                    signature=None)
                with contextlib.redirect_stdout(out):
                    code = m.main()
        return code, cls, out.getvalue()

    def test_defaults(self):
        code, cls, out = self._run(['/tmp/a.bin', '--bucket', 'my-bucket'])
        self.assertEqual(code, 0)
        # Uploader is constructed with the bucket + default KMS config.
        _, ctor = cls.call_args
        self.assertEqual(ctor['bucket'], 'my-bucket')
        self.assertEqual(ctor['kms_key'], m.DEFAULT_KMS_KEY)
        self.assertEqual(ctor['region'], m.DEFAULT_REGION)
        # And driven with the write-once + sign defaults.
        _, kwargs = cls.return_value.upload.call_args
        self.assertTrue(kwargs['immutable'])
        self.assertTrue(kwargs['sign'])
        # Result is emitted as JSON.
        self.assertEqual(json.loads(out)['url'], 'https://h/p/o')

    def test_bucket_is_required(self):
        with mock.patch.object(sys, 'argv', ['upload.py', '/tmp/a.bin']):
            with mock.patch.object(m, 'S3Uploader'):
                with self.assertRaises(SystemExit):
                    m.main()

    def test_mutable_and_no_sign_flags(self):
        _, cls, _ = self._run(
            ['/tmp/a.bin', '--bucket', 'b', '--mutable', '--no-sign'])
        _, kwargs = cls.return_value.upload.call_args
        self.assertFalse(kwargs['immutable'])
        self.assertFalse(kwargs['sign'])

    def test_sign_role_arn_from_flag(self):
        _, cls, _ = self._run(
            ['/tmp/a.bin', '--bucket', 'b', '--sign-role-arn', 'arn:flag'])
        _, ctor = cls.call_args
        self.assertEqual(ctor['sign_role_arn'], 'arn:flag')

    def test_sign_role_arn_defaults_from_env(self):
        with mock.patch.dict(m.os.environ,
                             {m.SIGN_ROLE_ARN_ENV: 'arn:from-env'}):
            _, cls, _ = self._run(['/tmp/a.bin', '--bucket', 'b'])
        _, ctor = cls.call_args
        self.assertEqual(ctor['sign_role_arn'], 'arn:from-env')

    def test_key_prefix_and_kms_overrides(self):
        _, cls, _ = self._run([
            '/tmp/a.bin', '--bucket', 'b', '--key', 'k/o', '--prefix', 'p',
            '--kms-key', 'alias/gpg/x', '--region', 'eu-west-1'
        ])
        # KMS/region overrides land on the constructor...
        _, ctor = cls.call_args
        self.assertEqual(ctor['kms_key'], 'alias/gpg/x')
        self.assertEqual(ctor['region'], 'eu-west-1')
        # ...while key/prefix are per-upload.
        _, kwargs = cls.return_value.upload.call_args
        self.assertEqual(kwargs['key'], 'k/o')
        self.assertEqual(kwargs['prefix'], 'p')


if __name__ == '__main__':
    unittest.main()

# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Upload a file to an S3 bucket with an integrity + authenticity envelope.

An `S3Uploader` is bound to one bucket (served at
`https://<bucket>.s3.brave.com`) and returns, per upload, the file's `sha256`,
`size_bytes`, a KMS `signature` over the digest, and the S3 `version_id`.
Uploads are write-once by default (`If-None-Match: *`) with a server-side
SHA256 checksum, so a misbehaving caller can neither overwrite an object nor
publish corrupted bytes.

Signing may require assuming a cross-account role. Its ARN is supplied at
runtime (`sign_role_arn`, defaulting from the `KMS_SIGN_ROLE_ARN` env var) so no
account id or role name is baked into this public file; when unset, `sign()`
uses the ambient credentials directly.
"""

from __future__ import annotations

import argparse
import base64
import dataclasses
import hashlib
import json
import logging
import mmap
import os
from pathlib import Path
import subprocess
import sys

# KMS signing parameters. The caller's ambient credentials must be able to
# `kms:Sign` with it.
DEFAULT_KMS_KEY = 'alias/gpg/checksums_pre_release_v2'

# The KMS signing algorithm.
KMS_ALGORITHM = 'RSASSA_PSS_SHA_256'

# AWS region the S3 and KMS calls target.
DEFAULT_REGION = 'us-west-2'

# Env var the CLI reads the signing role ARN from, so the account id / role name
# stay in the (private) caller's environment rather than in this public file.
SIGN_ROLE_ARN_ENV = 'KMS_SIGN_ROLE_ARN'

# Public URL template for our buckets: every one is served at
# `https://<bucket>.s3.brave.com`. Used to compose the result's `url`.
PUBLIC_URL_TEMPLATE = 'https://{bucket}.s3.brave.com'


def _check_output(*command, env: dict[str, str] | None = None) -> str:
    """Run *command*, logging the invocation, and return its captured stdout.

    stderr is inherited so the `aws` CLI's own diagnostics stream to the log on
    failure. *env* overrides the child environment when given (used to pass
    assumed-role credentials to a single call); the parent env is inherited when
    it is None.

    Raises:
        subprocess.CalledProcessError: if the command exits non-zero.
    """
    logging.info(' >>>> %s', ' '.join(str(a) for a in command))
    return subprocess.run(command,
                          check=True,
                          text=True,
                          stdout=subprocess.PIPE,
                          env=env).stdout


@dataclasses.dataclass(frozen=True)
class ArtifactSignature:
    """A KMS signature over a file's SHA-256 digest."""

    # KMS key the signature was produced with (alias or ARN).
    key_id: str

    # Base64-encoded signature bytes, as returned by `aws kms sign`. Always
    # produced with `KMS_ALGORITHM` over the file's SHA-256 digest.
    signature: str


@dataclasses.dataclass(frozen=True)
class UploadResult:
    """The authenticity + integrity envelope for an uploaded object."""

    # Bucket and key the object was written to.
    bucket: str
    key: str

    # Public URL the object is served at
    # (`https://<bucket>.s3.brave.com/<key>`).
    url: str

    # Integrity: hex SHA-256 and byte size of the uploaded bytes.
    sha256: str
    size_bytes: int

    # S3 object version id (present when bucket versioning is enabled), so a
    # consumer can pin an exact revision even of a mutable object.
    version_id: str | None

    # S3 ETag of the stored object.
    etag: str | None

    # KMS signature, or None when the upload was made with `sign=False`.
    signature: ArtifactSignature | None


# The fields of `UploadResult` that we want to be summarised.
_PUBLIC_FIELDS = ('bucket', 'key', 'url', 'sha256', 'size_bytes', 'version_id',
                  'etag')


def summarise(result: UploadResult) -> str:
    """Return a human-readable, publicly-safe summary of *result*.

    A user friendly summary of the upload result, suitable for printing in CI.
    """
    width = max(len(name) for name in _PUBLIC_FIELDS) + 1
    return '\n'.join(f'{name + ":":<{width}} {getattr(result, name)}'
                     for name in _PUBLIC_FIELDS
                     if getattr(result, name) is not None)


def sha256_file(path: Path) -> str:
    """Return the hex SHA-256 of *path*.

    The file is memory-mapped and hashed in a single `update`, leaving the OS
    to page bytes in and out of the page cache on demand rather than us
    buffering them in Python.
    """
    digest = hashlib.sha256()
    with path.open('rb') as file:
        if os.fstat(file.fileno()).st_size == 0:
            return digest.hexdigest()
        with mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ) as mapped:
            digest.update(mapped)
    return digest.hexdigest()


def kms_verify(sha256_hex: str,
               signature: ArtifactSignature,
               region: str = DEFAULT_REGION) -> None:
    """Verify a KMS *signature* over a SHA-256 digest, raising on failure.

    The download-side counterpart to `S3Uploader.sign`: a consumer verifying a
    downloaded file needs no bucket, so this is a free function rather than an
    `S3Uploader` method.

    Raises:
        subprocess.CalledProcessError: if the signature does not verify.
    """
    digest = bytes.fromhex(sha256_hex)
    message_b64 = base64.b64encode(digest).decode('ascii')
    _check_output('aws', 'kms', 'verify', '--region', region, '--key-id',
                  signature.key_id, '--signing-algorithm', KMS_ALGORITHM,
                  '--message-type', 'DIGEST', '--message', message_b64,
                  '--signature', signature.signature, '--output', 'json')


class S3Uploader:
    """Uploads files to one S3 bucket with a signed, verifiable envelope.

    Binds the destination bucket and the AWS region + KMS key shared across
    uploads. Each call `upload()` supplies the file and its key. Uses the
    `aws` CLI with ambient credentials (see the module docstring).
    """

    def __init__(self,
                 bucket: str,
                 region: str = DEFAULT_REGION,
                 kms_key: str = DEFAULT_KMS_KEY,
                 sign_role_arn: str | None = None):
        """Bind the uploader to *bucket* and its signing parameters.

        Args:
            bucket: Destination S3 bucket.
            region: AWS region for the S3 and KMS calls.
            kms_key: KMS key alias/ARN to sign with.
            sign_role_arn: Role ARN to assume before signing. Supplied at
                runtime so this public file carries no account id/role name;
                when None, `sign()` uses the ambient credentials directly.
        """
        self._bucket = bucket
        self._region = region
        self._kms_key = kms_key
        self._sign_role_arn = sign_role_arn

    def _assume_role_env(self) -> dict[str, str] | None:
        """Return an env with `sign_role_arn`'s temp creds, or None if unset.

        The KMS key's policy trusts a dedicated signing role rather than the
        caller's identity, so signing must run under assumed-role credentials.
        Assuming the role here (only for the sign call) leaves the ambient
        credentials, which the bucket policy grants `PutObject`, in place for
        the upload.
        """
        if not self._sign_role_arn:
            return None
        credentials = json.loads(
            _check_output('aws', 'sts', 'assume-role', '--region',
                          self._region, '--role-arn', self._sign_role_arn,
                          '--role-session-name', 'brave-upload-sign',
                          '--output', 'json'))['Credentials']
        return {
            **os.environ,
            'AWS_ACCESS_KEY_ID': credentials['AccessKeyId'],
            'AWS_SECRET_ACCESS_KEY': credentials['SecretAccessKey'],
            'AWS_SESSION_TOKEN': credentials['SessionToken'],
        }

    def sign(self, sha256_hex: str) -> ArtifactSignature:
        """Sign a SHA-256 digest with this uploader's KMS key.

        The digest is signed directly (`--message-type DIGEST`), with the raw
        32-byte digest passed base64-encoded as the message. When
        `sign_role_arn` is set the sign call runs under that assumed role (see
        `_assume_role_env`), otherwise it uses the ambient credentials.
        """
        digest = bytes.fromhex(sha256_hex)
        if len(digest) != 32:
            raise ValueError(
                f'Expected a 32-byte SHA-256 digest, got {len(digest)} bytes.')
        message_b64 = base64.b64encode(digest).decode('ascii')
        response = json.loads(
            _check_output('aws',
                          'kms',
                          'sign',
                          '--region',
                          self._region,
                          '--key-id',
                          self._kms_key,
                          '--signing-algorithm',
                          KMS_ALGORITHM,
                          '--message-type',
                          'DIGEST',
                          '--message',
                          message_b64,
                          '--output',
                          'json',
                          env=self._assume_role_env()))
        return ArtifactSignature(key_id=self._kms_key,
                                 signature=response['Signature'])

    def _put_object(self, path: Path, key: str,
                    immutable: bool) -> tuple[str | None, str | None]:
        """PUT *path* to `s3://<bucket>/<key>` and return `(version_id, etag)`.

        With *immutable* set, an `If-None-Match: *` precondition makes S3 reject
        the write if the key already exists, so an object is never overwritten
        in place. `--checksum-algorithm SHA256` has S3 validate the bytes.
        """
        command = [
            'aws', 's3api', 'put-object', '--region', self._region, '--bucket',
            self._bucket, '--key', key, '--body',
            str(path), '--checksum-algorithm', 'SHA256', '--output', 'json'
        ]
        if immutable:
            # Conditional write: fails with PreconditionFailed if the key
            # already exists.
            command += ['--if-none-match', '*']
        response = json.loads(_check_output(*command))
        return response.get('VersionId'), response.get('ETag')

    def upload(self,
               path: Path,
               key: str | None = None,
               prefix: str | None = None,
               immutable: bool = True,
               sign: bool = True) -> UploadResult:
        """Upload *path* and return its integrity + authenticity envelope.

        Args:
            path: The local file to upload.
            key: Explicit destination key. When omitted, the key is the file's
                name, optionally under *prefix* (`<prefix>/<filename>`).
            prefix: Key prefix used when *key* is not given. Ignored otherwise.
            immutable: Enforce write-once via `If-None-Match: *` (default). Pass
                False for objects meant to be replaced.
            sign: Produce a KMS signature over the file's digest (default).

        Returns:
            An `UploadResult` describing what was published.

        Raises:
            FileNotFoundError: if *path* does not exist.
            subprocess.CalledProcessError: if an `aws` call fails — notably a
                PreconditionFailed when *immutable* and the key already exists.
        """
        if not path.is_file():
            raise FileNotFoundError(f'File to upload not found: {path}')

        if key is not None:
            resolved_key = key
        elif prefix:
            resolved_key = f'{prefix.rstrip("/")}/{path.name}'
        else:
            resolved_key = path.name

        sha256_hex = sha256_file(path)
        size_bytes = path.stat().st_size

        logging.info('Uploading %s (%d bytes, sha256=%s) to s3://%s/%s%s',
                     path, size_bytes, sha256_hex, self._bucket, resolved_key,
                     ' [immutable]' if immutable else '')

        # Sign before the upload so a KMS/permissions failure aborts without
        # leaving an unsigned object behind.
        signature = self.sign(sha256_hex) if sign else None

        version_id, etag = self._put_object(path, resolved_key, immutable)

        host = PUBLIC_URL_TEMPLATE.format(bucket=self._bucket)
        url = f'{host}/{resolved_key}'
        return UploadResult(bucket=self._bucket,
                            key=resolved_key,
                            url=url,
                            sha256=sha256_hex,
                            size_bytes=size_bytes,
                            version_id=version_id,
                            etag=etag,
                            signature=signature)


def main() -> int:
    """CLI wrapper: upload one file and print its `UploadResult` as JSON."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('file', help='Local file to upload.')
    parser.add_argument('--bucket', required=True, help='Destination bucket.')
    parser.add_argument('--key', help='Explicit destination key.')
    parser.add_argument('--prefix',
                        help='Key prefix used when --key is not given.')
    parser.add_argument('--mutable',
                        action='store_true',
                        help='Allow overwriting an existing object (no '
                        'If-None-Match).')
    parser.add_argument('--no-sign',
                        action='store_true',
                        help='Skip KMS signing.')
    parser.add_argument('--kms-key', default=DEFAULT_KMS_KEY)
    parser.add_argument('--region', default=DEFAULT_REGION)
    parser.add_argument('--sign-role-arn',
                        default=os.environ.get(SIGN_ROLE_ARN_ENV),
                        help='Role ARN to assume before signing (defaults to '
                        f'${SIGN_ROLE_ARN_ENV}).')
    parser.add_argument('--verbose', action='store_true')
    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    uploader = S3Uploader(bucket=args.bucket,
                          region=args.region,
                          kms_key=args.kms_key,
                          sign_role_arn=args.sign_role_arn)
    result = uploader.upload(Path(args.file).expanduser().resolve(),
                             key=args.key,
                             prefix=args.prefix,
                             immutable=not args.mutable,
                             sign=not args.no_sign)
    print(json.dumps(dataclasses.asdict(result), indent=2))
    return 0


if __name__ == '__main__':
    sys.exit(main())

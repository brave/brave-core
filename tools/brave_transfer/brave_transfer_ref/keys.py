# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Sync-chain key derivation and discovery tags (PROTOCOL.md §3)."""

import hashlib
import hmac
import struct
from dataclasses import dataclass

from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric.x25519 import X25519PrivateKey
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat

from . import bip39

EPOCH_SECONDS = 900
TAG_LENGTH = 8


def hkdf_sha256(ikm: bytes, salt: bytes, info: bytes, length=32) -> bytes:
    return HKDF(algorithm=hashes.SHA256(),
                length=length,
                salt=salt or None,
                info=info).derive(ikm)


@dataclass(frozen=True)
class ChainKeys:
    root: bytes
    psk: bytes
    discovery: bytes

    @classmethod
    def from_seed(cls, seed: bytes) -> 'ChainKeys':
        if len(seed) != 32:
            raise ValueError('seed must be 32 bytes')
        root = hkdf_sha256(seed, b'brave-transfer', b'v1/root')
        return cls.from_root(root)

    @classmethod
    def from_root(cls, root: bytes) -> 'ChainKeys':
        if len(root) != 32:
            raise ValueError('root key must be 32 bytes')
        return cls(root=root,
                   psk=hkdf_sha256(root, b'', b'v1/chain-psk'),
                   discovery=hkdf_sha256(root, b'', b'v1/discovery'))

    @classmethod
    def from_sync_code(cls, sync_code: str) -> 'ChainKeys':
        return cls.from_seed(bip39.mnemonic_to_bytes32(sync_code))

    def tag(self, epoch: int) -> bytes:
        return hmac.new(self.discovery, struct.pack('>Q', epoch),
                        hashlib.sha256).digest()[:TAG_LENGTH]

    def accepted_tags(self, unix_seconds: float) -> list:
        epoch = epoch_for(unix_seconds)
        return [
            self.tag(e) for e in (epoch - 1, epoch, epoch + 1) if e >= 0
        ]


def epoch_for(unix_seconds: float) -> int:
    return int(unix_seconds // EPOCH_SECONDS)


def x25519_public(private: bytes) -> bytes:
    return X25519PrivateKey.from_private_bytes(private).public_key(
    ).public_bytes(Encoding.Raw, PublicFormat.Raw)


def fingerprint(static_public: bytes) -> str:
    digest = hashlib.sha256(static_public).hexdigest()[:32]
    return ' '.join(digest[i:i + 4] for i in range(0, 32, 4))

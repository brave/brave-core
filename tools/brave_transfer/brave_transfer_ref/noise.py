# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Noise_XXpsk3_25519_ChaChaPoly_SHA256 (Noise rev. 34), handshake only.

A deliberately small, readable implementation of exactly the pattern Brave
Transfer uses. Checked against the public cacophony vector in
test/data/noise_cacophony_xxpsk3.json.
"""

import hashlib
import hmac
import os
import struct

from cryptography.exceptions import InvalidTag
from cryptography.hazmat.primitives.asymmetric.x25519 import (X25519PrivateKey,
                                                              X25519PublicKey)
from cryptography.hazmat.primitives.ciphers.aead import ChaCha20Poly1305

from .keys import x25519_public

PROTOCOL_NAME = b'Noise_XXpsk3_25519_ChaChaPoly_SHA256'
DHLEN = 32
HASHLEN = 32
TAGLEN = 16
MAX_MESSAGE = 65535

# XXpsk3. Each message is a list of tokens, alternating initiator/responder.
PATTERN = (('e', ), ('e', 'ee', 's', 'es'), ('s', 'se', 'psk'))


class NoiseError(Exception):
    pass


def _hmac(key: bytes, data: bytes) -> bytes:
    return hmac.new(key, data, hashlib.sha256).digest()


def _hkdf(chaining_key: bytes, ikm: bytes, outputs: int):
    temp = _hmac(chaining_key, ikm)
    out1 = _hmac(temp, b'\x01')
    out2 = _hmac(temp, out1 + b'\x02')
    if outputs == 2:
        return out1, out2
    return out1, out2, _hmac(temp, out2 + b'\x03')


def _dh(private: bytes, public: bytes) -> bytes:
    try:
        shared = X25519PrivateKey.from_private_bytes(private).exchange(
            X25519PublicKey.from_public_bytes(public))
    except ValueError as e:
        raise NoiseError('low-order public key') from e
    if shared == bytes(DHLEN):
        raise NoiseError('low-order public key')
    return shared


class CipherState:

    def __init__(self, key=None):
        self.key = key
        self.nonce = 0

    def has_key(self):
        return self.key is not None

    def _nonce(self):
        if self.nonce >= 2**64 - 1:
            raise NoiseError('nonce exhausted')
        n = b'\x00' * 4 + struct.pack('<Q', self.nonce)
        self.nonce += 1
        return n

    def encrypt(self, ad: bytes, plaintext: bytes) -> bytes:
        if self.key is None:
            return plaintext
        return ChaCha20Poly1305(self.key).encrypt(self._nonce(), plaintext,
                                                  ad)

    def decrypt(self, ad: bytes, ciphertext: bytes) -> bytes:
        if self.key is None:
            return ciphertext
        try:
            return ChaCha20Poly1305(self.key).decrypt(self._nonce(),
                                                      ciphertext, ad)
        except InvalidTag as e:
            raise NoiseError('decryption failed') from e


class SymmetricState:

    def __init__(self):
        name = PROTOCOL_NAME
        self.h = name.ljust(HASHLEN, b'\x00') if len(
            name) <= HASHLEN else hashlib.sha256(name).digest()
        self.ck = self.h
        self.cipher = CipherState()

    def mix_key(self, ikm):
        self.ck, temp_k = _hkdf(self.ck, ikm, 2)
        self.cipher = CipherState(temp_k)

    def mix_hash(self, data):
        self.h = hashlib.sha256(self.h + data).digest()

    def mix_key_and_hash(self, ikm):
        self.ck, temp_h, temp_k = _hkdf(self.ck, ikm, 3)
        self.mix_hash(temp_h)
        self.cipher = CipherState(temp_k)

    def encrypt_and_hash(self, plaintext):
        ciphertext = self.cipher.encrypt(self.h, plaintext)
        self.mix_hash(ciphertext)
        return ciphertext

    def decrypt_and_hash(self, ciphertext):
        plaintext = self.cipher.decrypt(self.h, ciphertext)
        self.mix_hash(ciphertext)
        return plaintext

    def split(self):
        k1, k2 = _hkdf(self.ck, b'', 2)
        return CipherState(k1), CipherState(k2)


class HandshakeState:
    """One side of an XXpsk3 handshake.

    `ephemeral` may be fixed for test vectors; it is random otherwise.
    """

    def __init__(self,
                 initiator: bool,
                 prologue: bytes,
                 static_private: bytes,
                 psk: bytes = None,
                 ephemeral_private: bytes = None):
        self.initiator = initiator
        self.symmetric = SymmetricState()
        self.symmetric.mix_hash(prologue)
        self.s = static_private
        self.s_pub = x25519_public(static_private)
        self.e = ephemeral_private
        self.psk = psk
        self.rs = None
        self.re = None
        self.message_index = 0
        self.send_cipher = None
        self.recv_cipher = None
        self.handshake_hash = None

    @property
    def complete(self):
        return self.message_index == len(PATTERN)

    def set_psk(self, psk: bytes):
        if len(psk) != 32:
            raise NoiseError('psk must be 32 bytes')
        self.psk = psk

    def _my_turn(self):
        return (self.message_index % 2 == 0) == self.initiator

    def _dh_token(self, token):
        # Token names are from the initiator's point of view.
        if token == 'ee':
            return _dh(self.e, self.re)
        if token == 'es':
            return _dh(self.e, self.rs) if self.initiator else _dh(
                self.s, self.re)
        if token == 'se':
            return _dh(self.s, self.re) if self.initiator else _dh(
                self.e, self.rs)
        raise AssertionError(token)

    def write_message(self, payload: bytes) -> bytes:
        if self.complete or not self._my_turn():
            raise NoiseError('not our turn to write')
        sym = self.symmetric
        out = b''
        for token in PATTERN[self.message_index]:
            if token == 'e':
                if self.e is None:
                    self.e = os.urandom(32)
                e_pub = x25519_public(self.e)
                out += e_pub
                sym.mix_hash(e_pub)
                sym.mix_key(e_pub)  # psk modifier
            elif token == 's':
                out += sym.encrypt_and_hash(self.s_pub)
            elif token == 'psk':
                if self.psk is None:
                    raise NoiseError('psk not set')
                sym.mix_key_and_hash(self.psk)
            else:
                sym.mix_key(self._dh_token(token))
        out += sym.encrypt_and_hash(payload)
        if len(out) > MAX_MESSAGE:
            raise NoiseError('message too long')
        self._advance()
        return out

    def read_message(self, message: bytes) -> bytes:
        if self.complete or self._my_turn():
            raise NoiseError('not our turn to read')
        if len(message) > MAX_MESSAGE:
            raise NoiseError('message too long')
        sym = self.symmetric
        pos = 0
        for token in PATTERN[self.message_index]:
            if token == 'e':
                if len(message) - pos < DHLEN:
                    raise NoiseError('truncated message')
                self.re = message[pos:pos + DHLEN]
                pos += DHLEN
                sym.mix_hash(self.re)
                sym.mix_key(self.re)
            elif token == 's':
                n = DHLEN + (TAGLEN if sym.cipher.has_key() else 0)
                if len(message) - pos < n:
                    raise NoiseError('truncated message')
                self.rs = sym.decrypt_and_hash(message[pos:pos + n])
                pos += n
            elif token == 'psk':
                if self.psk is None:
                    raise NoiseError('psk not set')
                sym.mix_key_and_hash(self.psk)
            else:
                sym.mix_key(self._dh_token(token))
        if sym.cipher.has_key() and len(message) - pos < TAGLEN:
            raise NoiseError('truncated message')
        payload = sym.decrypt_and_hash(message[pos:])
        self._advance()
        return payload

    def _advance(self):
        self.message_index += 1
        if self.complete:
            c1, c2 = self.symmetric.split()
            self.send_cipher, self.recv_cipher = (c1, c2) if self.initiator \
                else (c2, c1)
            self.handshake_hash = self.symmetric.h

# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""BIP39 English mnemonic <-> 32-byte entropy.

Mirrors brave_sync::crypto::PassphraseFromBytes32 / PassphraseToBytes32, which
call libwally's bip39_mnemonic_from_bytes / bip39_mnemonic_to_bytes. Only
24-word codes (256-bit entropy, 8-bit checksum) are valid sync codes.
"""

import hashlib
from pathlib import Path

WORDS = Path(__file__).with_name('bip39_english.txt').read_text().split()
assert len(WORDS) == 2048
_INDEX = {word: i for i, word in enumerate(WORDS)}


class InvalidMnemonic(ValueError):
    pass


def bytes32_to_mnemonic(entropy: bytes) -> str:
    if len(entropy) != 32:
        raise ValueError('entropy must be 32 bytes')
    checksum = hashlib.sha256(entropy).digest()[0]
    bits = (int.from_bytes(entropy, 'big') << 8) | checksum
    return ' '.join(WORDS[(bits >> (11 * (23 - i))) & 0x7FF]
                    for i in range(24))


def mnemonic_to_bytes32(mnemonic: str) -> bytes:
    words = mnemonic.split(' ')
    if len(words) != 24:
        raise InvalidMnemonic('a sync code has exactly 24 words')
    bits = 0
    for word in words:
        if word not in _INDEX:
            raise InvalidMnemonic(f'unknown word {word!r}')
        bits = (bits << 11) | _INDEX[word]
    entropy = (bits >> 8).to_bytes(32, 'big')
    if hashlib.sha256(entropy).digest()[0] != bits & 0xFF:
        raise InvalidMnemonic('bad checksum')
    return entropy

# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Receiver-side filename sanitization (PROTOCOL.md §7)."""

MAX_NAME_BYTES = 255
MAX_EXTENSION_BYTES = 32
FALLBACK_NAME = 'file'

_REPLACED = set('/\\<>:"|?*')
_RESERVED = {'CON', 'PRN', 'AUX', 'NUL'} | {f'COM{i}' for i in range(1, 10)
                                            } | {f'LPT{i}' for i in range(1, 10)}


def _is_removed(cp: int) -> bool:
    return (cp <= 0x1F or 0x7F <= cp <= 0x9F or cp == 0x061C
            or 0x200B <= cp <= 0x200F or 0x202A <= cp <= 0x202E
            or 0x2060 <= cp <= 0x2069 or cp == 0xFEFF)


def _truncate_utf8(text: str, max_bytes: int) -> str:
    out, used = [], 0
    for ch in text:
        n = len(ch.encode('utf-8'))
        if used + n > max_bytes:
            break
        out.append(ch)
        used += n
    return ''.join(out)


def sanitize_filename(name: str) -> str:
    chars = []
    for ch in name:
        if _is_removed(ord(ch)):
            continue
        chars.append('_' if ch in _REPLACED else ch)
    result = ''.join(chars).strip(' .')
    if not result:
        return FALLBACK_NAME
    # Windows ignores trailing spaces, so "aux .txt" is reserved too.
    stem = result.split('.', 1)[0].rstrip(' ')
    if ''.join(c.upper() if c.isascii() else c
               for c in stem) in _RESERVED:
        result = '_' + result
    if len(result.encode('utf-8')) > MAX_NAME_BYTES:
        dot = result.rfind('.')
        ext = result[dot:] if dot > 0 else ''
        if len(ext.encode('utf-8')) > MAX_EXTENSION_BYTES:
            ext = ''
        stem = result[:len(result) - len(ext)]
        result = _truncate_utf8(stem, MAX_NAME_BYTES -
                                len(ext.encode('utf-8'))).rstrip('. ') + ext
    return result

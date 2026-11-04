# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for generate_root_cert_list.py.

python3 brave/net/cert/generate_root_cert_list_test.py
"""

import base64
import hashlib
import json
import re
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import generate_root_cert_list as gen

SRC_DIR = Path(__file__).resolve().parents[3]
ROOT_STORES = SRC_DIR / 'net/data/ssl/root_stores/root_stores.json'
CRS_TEXTPROTO = SRC_DIR / 'net/data/ssl/chrome_root_store/root_store.textproto'
CRS_CERTS = SRC_DIR / 'net/data/ssl/chrome_root_store/root_store.certs'

# Sectigo Public Server Authentication Root R46: in the Chrome Root Store,
# missing from root_stores.json.
R46_CERT_HASH = (
    '7bb647a62aeeac88bf257aa522d01ffea395e0ab45c73f93f65654ec38f25a06'
)
R46_SPKI_HASH = (
    '0e8bb18bbeefb381be21bfc1a206d317298462ad104855f04a0542699708d3d4'
)
R46_CRS_ROOT_ID = 95


def _certs_by_hash() -> dict[str, bytes]:
    certs = {}
    for pem in gen.PEM_RE.findall(CRS_CERTS.read_text()):
        der = base64.b64decode(''.join(pem.split()))
        certs[hashlib.sha256(der).hexdigest()] = der
    return certs


def _pem(der: bytes) -> str:
    body = base64.b64encode(der).decode()
    lines = [body[i : i + 64] for i in range(0, len(body), 64)]
    return '\n'.join(
        ['-----BEGIN CERTIFICATE-----', *lines, '-----END CERTIFICATE-----', '']
    )


class GenerateRootCertListTest(unittest.TestCase):
    def setUp(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)

    def tearDown(self) -> None:
        self._tmp.cleanup()

    def _write(self, name: str, content: str) -> Path:
        path = self.tmp / name
        path.write_text(content)
        return path

    def test_extract_spki_matches_root_stores_json(self) -> None:
        # root_stores.json independently maps cert fingerprints to SPKI hashes.
        expected = {
            fingerprint: spki
            for spki, data in json.loads(ROOT_STORES.read_text())[
                'spkis'
            ].items()
            for fingerprint in data['fingerprints']
        }
        checked = 0
        for cert_hash, der in _certs_by_hash().items():
            if cert_hash not in expected:
                continue
            spki_hash = hashlib.sha256(gen.extract_spki(der)).hexdigest()
            self.assertEqual(expected[cert_hash], spki_hash, cert_hash)
            checked += 1
        self.assertGreater(checked, 0)

    def test_merge_keeps_every_legacy_entry_and_id(self) -> None:
        legacy = json.loads(ROOT_STORES.read_text())['spkis']
        merged = gen.merge(ROOT_STORES, CRS_TEXTPROTO, CRS_CERTS)
        for spki, data in legacy.items():
            self.assertEqual(int(data['id']), merged.get(spki), spki)

    def test_merge_adds_crs_roots_missing_from_legacy(self) -> None:
        legacy = json.loads(ROOT_STORES.read_text())['spkis']
        self.assertNotIn(R46_SPKI_HASH, legacy)
        merged = gen.merge(ROOT_STORES, CRS_TEXTPROTO, CRS_CERTS)
        self.assertEqual(
            gen.CRS_HISTOGRAM_ID_BASE + R46_CRS_ROOT_ID, merged[R46_SPKI_HASH]
        )

    def test_merge_includes_every_crs_trust_anchor(self) -> None:
        anchors = gen.parse_trust_anchors(CRS_TEXTPROTO)
        certs = _certs_by_hash()
        merged = gen.merge(ROOT_STORES, CRS_TEXTPROTO, CRS_CERTS)
        for cert_hash in anchors:
            spki_hash = hashlib.sha256(
                gen.extract_spki(certs[cert_hash])
            ).hexdigest()
            self.assertIn(spki_hash, merged, cert_hash)

    def test_parse_ignores_non_anchor_blocks(self) -> None:
        textproto = self._write(
            'store.textproto',
            f"""
            version_major: 1
            trust_anchors {{
              sha256_hex: "{'a' * 64}"
              crs_root_id: 1
              constraints: {{
                min_version: "1.0"
              }}
            }}
            additional_certs {{
              sha256_hex: "{'b' * 64}"
              crs_root_id: 2
            }}
            """,
        )
        self.assertEqual({'a' * 64: 1}, gen.parse_trust_anchors(textproto))

    def test_parse_rejects_anchor_without_crs_root_id(self) -> None:
        textproto = self._write(
            'store.textproto',
            f'trust_anchors {{\n  sha256_hex: "{"a" * 64}"\n}}\n',
        )
        with self.assertRaises(ValueError):
            gen.parse_trust_anchors(textproto)

    def test_missing_certificate_raises(self) -> None:
        textproto = self._write(
            'store.textproto',
            f'trust_anchors {{\n  sha256_hex: "{"a" * 64}"\n'
            '  crs_root_id: 1\n}\n',
        )
        certs = self._write('store.certs', '')
        with self.assertRaisesRegex(ValueError, 'No certificate'):
            gen.crs_spki_ids(textproto, certs)

    def test_histogram_id_overflow_raises(self) -> None:
        textproto = self._write(
            'store.textproto',
            f'trust_anchors {{\n  sha256_hex: "{R46_CERT_HASH}"\n'
            '  crs_root_id: 7000\n}\n',
        )
        certs = self._write(
            'store.certs', _pem(_certs_by_hash()[R46_CERT_HASH])
        )
        root_stores = self._write('root_stores.json', '{"spkis": {}}')
        with self.assertRaisesRegex(ValueError, 'overflows'):
            gen.merge(root_stores, textproto, certs)

    def test_render_sorts_by_spki(self) -> None:
        output = gen.render({'ff' * 32: 2, '00' * 32: 1})
        ids = [int(m.group(1)) for m in re.finditer(r'\}, (\d+)\},', output)]
        self.assertEqual([1, 2], ids)
        self.assertTrue(output.startswith(gen.HEADER))
        self.assertTrue(output.endswith(gen.FOOTER))


if __name__ == '__main__':
    unittest.main()

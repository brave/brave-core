#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the Mozilla Public License, v. 2.0.

"""Split bundled notice inventories into independent brave://credits entries.

Reads only checked-in files. The aggregate resources remain available to the
native notices viewer; credits references only the individual license files.
"""

import argparse
from pathlib import Path
import re

DIVIDER = '\n\n' + '-' * 20 + ' DEPENDENCY DIVIDER ' + '-' * 20 + '\n\n'
NATIVE = [
    ('Tor', 'https://www.torproject.org/', '0.4.9.14', 'BSD-3-Clause'),
    ('OpenSSL', 'https://www.openssl.org/', '3.6.5', 'Apache-2.0'),
    ('libevent', 'https://libevent.org/', '2.1.13', 'BSD-3-Clause'),
    ('liblzma', 'https://tukaani.org/xz/', '5.8.4', '0BSD'),
    ('Tor.framework', 'https://github.com/iCepa/Tor.framework', '409.14.1', 'MIT'),
]
TRANSPORT_NAMES = {
    'lyrebird': 'Lyrebird',
    'snowflake/v2': 'Snowflake',
    'goptlib': 'goptlib',
    'ptutil': 'ptutil',
    'webtunnel': 'WebTunnel',
}


def entry(name, url, version, text, license_id=None):
    slug = re.sub(r'[^a-z0-9]+', '-', name.lower()).strip('-')
    filename = f'Sources/BraveTorUI/Resources/Credits/{slug}.txt'
    metadata = (
        f'Name: {name}\nURL: {url}\nVersion: {version}\n'
        f'License: {license_id or "LicenseRef-" + slug}\n'
        f'License File: //brave/ios/tor/{filename}\n'
        'Shipped: yes\nSecurity Critical: yes\n'
    )
    return filename, text.rstrip() + '\n', metadata


def generate(root):
    resources = root / 'Sources/BraveTorUI/Resources'
    native = (resources / 'ThirdPartyNotices.txt').read_text(encoding='utf-8')
    sections = re.split(r'\n={72}\n\n', native)[1:]
    if len(sections) != len(NATIVE):
        raise ValueError('Unexpected native notice boundaries')
    entries = [entry(name, url, version, text, license_id)
               for (name, url, version, license_id), text in zip(NATIVE, sections)]
    native_count = len(entries)
    transport = (resources / 'TransportNotices.txt').read_text(encoding='utf-8')
    transport = transport[transport.index('IPtProxy 5.7.0\n'):]
    sections = re.split(r'\n={72}\n\n(?=Go runtime go\d|\S+ v\d)', transport)
    for index, text in enumerate(sections):
        module, version = text.splitlines()[0].split(' ', 1)
        if index == 0:
            name, url, license_id = 'IPtProxy', 'https://github.com/tladesignz/IPtProxy', 'MIT'
        elif index == 1:
            if module != 'Go' or not version.startswith('runtime '):
                raise ValueError('Missing Go runtime notice')
            name, url, license_id = 'Go runtime', 'https://go.dev/', 'BSD-3-Clause'
            version = version.removeprefix('runtime ')
        else:
            short = module.removeprefix('gitlab.torproject.org/tpo/anti-censorship/pluggable-transports/')
            name = TRANSPORT_NAMES.get(short, module)
            url, license_id = f'https://pkg.go.dev/{module}@{version}', None
        entries.append(entry(name, url, version, text, license_id))
    # Every module archive must have exactly one independent entry.
    if len(entries) != native_count + 2 + transport.count('Archive SHA-256:'):
        raise ValueError('Incomplete transport notice inventory')
    files = {path: text for path, text, _ in entries}
    if len(files) != len(entries):
        raise ValueError('Duplicate credit entry name')
    for group, items in [('embedded', entries[:native_count]),
                         ('transports', entries[native_count:])]:
        files[f'credits/{group}/README.chromium'] = DIVIDER.join(item[2].rstrip() for item in items) + '\n'
    return files


def write_entries(root, check=False):
    files = generate(root)
    for name, content in files.items():
        path = root / name
        if check:
            if not path.is_file() or path.read_text(encoding='utf-8') != content:
                raise ValueError(f'Outdated credits: {name}')
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding='utf-8')
    print(f'{len(files) - 2} individual credit entries verified' if check
          else f'Wrote {len(files) - 2} individual credit entries')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    write_entries(Path(__file__).resolve().parents[1], args.check)

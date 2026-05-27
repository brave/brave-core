#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
# Wallet hardcoded typography -> leo.font tokens.

import re
from pathlib import Path
from typing import Optional

ROOT = Path(__file__).resolve().parents[1]

SIZE_WEIGHT_TO_FONT = {
    ('10px', '400'): 'xSmall.regular',
    ('10px', '500'): 'xSmall.semibold',
    ('10px', 'normal'): 'xSmall.regular',
    ('11px', '400'): 'xSmall.regular',
    ('11px', '600'): 'xSmall.semibold',
    ('12px', '400'): 'small.regular',
    ('12px', 'normal'): 'small.regular',
    ('12px', '600'): 'small.semibold',
    ('13px', '400'): 'small.regular',
    ('13px', '600'): 'components.tableheader',
    ('14px', '400'): 'default.regular',
    ('14px', 'normal'): 'default.regular',
    ('14px', '600'): 'default.semibold',
    ('15px', '400'): 'default.regular',
    ('15px', '600'): 'heading.h4',
    ('16px', '400'): 'large.regular',
    ('16px', '600'): 'large.semibold',
    ('16px', '500'): 'large.semibold',
    ('18px', '400'): 'large.regular',
    ('18px', '600'): 'large.semibold',
    ('18px', '500'): 'large.semibold',
    ('20px', '400'): 'large.regular',
    ('20px', '600'): 'heading.h3',
    ('20px', '500'): 'heading.h3',
    ('22px', '600'): 'heading.h2',
    ('22px', '500'): 'heading.h2',
    ('24px', '600'): 'heading.h2',
    ('28px', '600'): 'heading.h1',
    ('32px', '600'): 'heading.display3',
    ('36px', '500'): 'components.numbersLarge',
    ('36px', '600'): 'components.numbersLarge',
    ('40px', '600'): 'heading.display2',
    ('46px', '600'): 'heading.display1',
    ('24px', '400'): 'heading.h2',
    ('24px', '600'): 'heading.h2',
    ('28px', '400'): 'heading.h1',
    ('28px', '600'): 'heading.h1',
    ('32px', '600'): 'heading.display3',
    ('34px', '600'): 'heading.display3',
    ('45px', '600'): 'heading.display2',
    ('45px', '500'): 'heading.display2',
}

LONE_FONT_SIZE = {
    '10px': 'xSmall.regular',
    '11px': 'xSmall.regular',
    '12px': 'small.regular',
    '13px': 'small.regular',
    '14px': 'default.regular',
    '15px': 'heading.h4',
    '16px': 'large.regular',
    '18px': 'large.regular',
    '20px': 'heading.h3',
    '22px': 'heading.h2',
    '24px': 'heading.h2',
    '28px': 'heading.h1',
    '32px': 'heading.display3',
    '34px': 'heading.display3',
    '36px': 'components.numbersLarge',
    '45px': 'heading.display2',
}

TEXT_SIZE_REPLACEMENTS = [
    ("textSize='10px'", "variant='xSmall.regular'"),
    ('textSize="10px"', 'variant="xSmall.regular"'),
    ("textSize='11px'", "variant='xSmall.regular'"),
    ('textSize="11px"', 'variant="xSmall.regular"'),
    ("textSize='12px'", "variant='small.regular'"),
    ('textSize="12px"', 'variant="small.regular"'),
    ("textSize='14px'", "variant='default.regular'"),
    ('textSize="14px"', 'variant="default.regular"'),
    ("textSize='16px'", "variant='large.regular'"),
    ('textSize="16px"', 'variant="large.regular"'),
    ("textSize='18px'", "variant='large.regular'"),
    ('textSize="18px"', 'variant="large.regular"'),
    ("textSize='20px'", "variant='heading.h3'"),
    ('textSize="20px"', 'variant="heading.h3"'),
    ("textSize='22px'", "variant='heading.h2'"),
    ('textSize="22px"', 'variant="heading.h2"'),
]

FONT_PROPS_RE = re.compile(
    r'(?:font-style:\s*[^;]+;\s*)?'
    r'font-size:\s*(\d+px);\s*'
    r'(?:font-weight:\s*(\d+|normal);\s*)?'
    r'(?:line-height:\s*[^;]+;\s*)?'
    r'|'
    r'font-size:\s*(\d+px);\s*'
    r'line-height:\s*[^;]+;\s*'
    r'(?:font-weight:\s*(\d+|normal);\s*)?'
    r'|'
    r'font-size:\s*(\d+px);\s*'
    r'line-height:\s*[^;]+;\s*'
    r'|'
    r'font-weight:\s*(\d+|normal);\s*'
    r'font-size:\s*(\d+px);\s*'
    r'(?:line-height:\s*[^;]+;\s*)?',
    re.MULTILINE,
)


def token_for(size: str, weight: Optional[str]) -> Optional[str]:
    w = weight or '400'
    return SIZE_WEIGHT_TO_FONT.get((size, w))


def replace_font_blocks(content: str) -> tuple[str, int]:
    count = 0

    patterns = [
        re.compile(
            r'font-style:\s*normal;\s*'
            r'font-weight:\s*(\d+|normal);\s*'
            r'font-size:\s*(\d+px);\s*'
            r'line-height:\s*[^;]+;\s*',
            re.MULTILINE,
        ),
        re.compile(
            r'font-style:\s*normal;\s*'
            r'font-size:\s*(\d+px);\s*'
            r'font-weight:\s*(\d+|normal);\s*'
            r'line-height:\s*[^;]+;\s*',
            re.MULTILINE,
        ),
        re.compile(
            r'font-size:\s*(\d+px);\s*'
            r'line-height:\s*[^;]+;\s*'
            r'font-weight:\s*(\d+|normal);\s*',
            re.MULTILINE,
        ),
        re.compile(
            r'font-size:\s*(\d+px);\s*'
            r'font-weight:\s*(\d+|normal);\s*'
            r'line-height:\s*[^;]+;\s*',
            re.MULTILINE,
        ),
        re.compile(
            r'font-size:\s*(\d+px);\s*'
            r'line-height:\s*[^;]+;\s*',
            re.MULTILINE,
        ),
        re.compile(
            r'font-weight:\s*(\d+|normal);\s*'
            r'font-size:\s*(\d+px);\s*'
            r'line-height:\s*[^;]+;\s*',
            re.MULTILINE,
        ),
        re.compile(
            r'font-size:\s*(\d+px);\s*'
            r'font-weight:\s*(\d+|normal);\s*',
            re.MULTILINE,
        ),
    ]

    lone_pat = re.compile(r'font-size:\s*(\d+px);\s*(?!\n\s*font-weight)',
                          re.MULTILINE)

    def lone_repl(m):
        nonlocal count
        token = LONE_FONT_SIZE.get(m.group(1))
        if not token:
            return m.group(0)
        count += 1
        return f'font: ${{leo.font.{token}}};\n'

    content = lone_pat.sub(lone_repl, content)

    for pat in patterns:

        def repl(m):
            nonlocal count
            gs = m.groups()
            if len(gs) == 1:
                size, weight = gs[0], '400'
            elif gs[0] and str(gs[0]).endswith('px'):
                size, weight = gs[0], gs[1] if len(gs) > 1 else '400'
            else:
                weight, size = gs[0], gs[1]
            weight = '400' if weight == 'normal' else weight
            token = token_for(size, weight)
            if not token:
                return m.group(0)
            count += 1
            return f'font: ${{leo.font.{token}}};\n'

        content = pat.sub(repl, content)

    return content, count


def ensure_leo_import(content: str) -> str:
    if '${leo.font.' not in content:
        return content
    if "import * as leo from '@brave/leo/tokens/css/variables'" in content:
        return content
    m = re.search(r"(import styled[^\n]+\n)", content)
    if m:
        return (content[:m.end()] +
                "import * as leo from '@brave/leo/tokens/css/variables'\n" +
                content[m.end():])
    return content


def migrate_tsx(content: str) -> str:
    for old, new in TEXT_SIZE_REPLACEMENTS:
        content = content.replace(old, new)
    content = re.sub(
        r"variant='(\w+)\.regular'(\s+)isBold=\{false\}",
        lambda m: f"variant='{m.group(1)}.regular'",
        content,
    )
    content = re.sub(
        r'variant="(\w+)\.regular"(\s+)isBold=\{false\}',
        lambda m: f'variant="{m.group(1)}.regular"',
        content,
    )
    content = re.sub(
        r"variant='(\w+)\.regular'(\s+)isBold(?:=\{true\})?",
        lambda m: f"variant='{m.group(1)}.semibold'",
        content,
    )
    content = re.sub(
        r'variant="(\w+)\.regular"(\s+)isBold(?:=\{true\})?',
        lambda m: f'variant="{m.group(1)}.semibold"',
        content,
    )
    # Fix any prior broken runs that left ={false} suffix.
    content = re.sub(
        r"variant='(\w+)\.semibold'=\{false\}",
        r"variant='\1.regular'",
        content,
    )
    content = re.sub(
        r'variant="(\w+)\.semibold"=\{false\}',
        r'variant="\1.regular"',
        content,
    )
    content = re.sub(
        r"(variant='[^']+\.semibold')\s+isBold(?:={true})?",
        r"\1",
        content,
    )
    content = re.sub(
        r'(variant="[^"]+\.semibold")\s+isBold(?:={true})?',
        r'\1',
        content,
    )
    return content


def process_file(path: Path) -> bool:
    content = path.read_text()
    original = content
    content, _ = replace_font_blocks(content)
    if path.suffix == '.tsx':
        content = migrate_tsx(content)
    content = ensure_leo_import(content)
    if content != original:
        path.write_text(content)
        return True
    return False


def main():
    changed = []
    for pattern in (
            '**/*.style.ts',
            '**/*.style.tsx',
            '**/*.styles.ts',
            '**/*.styles.tsx',
            '**/style.ts',
            '**/style.tsx',
            '**/*.tsx',
    ):
        for path in ROOT.glob(pattern):
            if 'scripts' in path.parts or 'node_modules' in str(path):
                continue
            if process_file(path):
                changed.append(path)
    print(f'Updated {len(changed)} files')


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
# Remove redundant font-weight after leo.font shorthand; fix split replacements.

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

UPGRADE_WEIGHT = {
    ('large.regular', '600'): 'large.semibold',
    ('large.regular', '500'): 'large.semibold',
    ('default.regular', '600'): 'default.semibold',
    ('small.regular', '600'): 'small.semibold',
    ('xSmall.regular', '600'): 'xSmall.semibold',
    ('small.regular', '500'): 'small.semibold',
}

FONT_STYLE_MIDDLE = re.compile(
    r'font-size:\s*(\d+px);\s*'
    r'font-style:\s*normal;\s*'
    r'font-weight:\s*(\d+|normal);\s*'
    r'line-height:\s*[^;]+;\s*',
    re.MULTILINE,
)

SIZE_WEIGHT = {
    ('12px', '600'): 'small.semibold',
    ('12px', '400'): 'small.regular',
    ('14px', '500'): 'default.semibold',
    ('14px', '600'): 'default.semibold',
}


def fix_font_style_middle(content: str) -> str:
    def repl(m):
        token = SIZE_WEIGHT.get((m.group(1), m.group(2)))
        if not token:
            return m.group(0)
        return f'font: ${{leo.font.{token}}};\n'
    return FONT_STYLE_MIDDLE.sub(repl, content)


def cleanup_orphan_weights(content: str) -> str:
    lines = content.split('\n')
    out = []
    i = 0
    while i < len(lines):
        line = lines[i]
        m = re.search(r'font:\s*\$\{leo\.font\.([^}]+)\}', line)
        if m:
            variant = m.group(1)
            out.append(line)
            i += 1
            # look ahead for font-weight within next 5 lines
            j = i
            while j < len(lines) and j < i + 6:
                wm = re.match(r'\s*font-weight:\s*(\d+);\s*$', lines[j])
                if wm:
                    weight = wm.group(1)
                    upgraded = UPGRADE_WEIGHT.get((variant, weight))
                    if upgraded:
                        out[-1] = re.sub(
                            r'leo\.font\.[^}]+\}',
                            f'leo.font.{upgraded}}}',
                            out[-1],
                        )
                    elif weight == '400':
                        pass  # drop redundant
                    else:
                        out.append(lines[j])
                    j += 1
                    i = j
                    break
                if re.search(r'font:\s*\$\{leo', lines[j]):
                    break
                j += 1
            else:
                continue
        else:
            out.append(line)
            i += 1
    return '\n'.join(out)


def fix_broken_indent(content: str) -> str:
    content = re.sub(
        r'(font: \$\{leo\.font\.[^}]+\};)\n(color:)',
        r'\1\n  \2',
        content,
    )
    content = re.sub(
        r'(font: \$\{leo\.font\.[^}]+\};)\n(text-align:)',
        r'\1\n  \2',
        content,
    )
    content = re.sub(
        r'(font: \$\{leo\.font\.[^}]+\};)\n(display:)',
        r'\1\n  \2',
        content,
    )
    return content


def remove_style_weight_before_font(content: str) -> str:
    def repl(m):
        weight = m.group(1)
        variant = m.group(2)
        if weight in ('600', '500'):
            upgraded = UPGRADE_WEIGHT.get((variant, weight))
            if upgraded:
                variant = upgraded
            elif weight == '600' and '.semibold' not in variant:
                if variant.endswith('.regular'):
                    variant = variant.replace('.regular', '.semibold')
        return f'font: ${{leo.font.{variant}}};\n'

    return re.sub(
        r'font-style:\s*normal;\s*'
        r'font-weight:\s*(\d+);\s*'
        r'font:\s*\$\{leo\.font\.([^}]+)\};\s*',
        repl,
        content,
    )


def fix_responsive_font_size(content: str) -> str:
    return re.sub(
        r'font-size:\s*12px;',
        'font: ${leo.font.small.regular};',
        content,
    )


def remove_orphan_font_lines(content: str) -> str:
    lines = content.split('\n')
    out = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if re.match(
            r'\s*(?:font-weight|line-height|font-style):\s*',
            line,
        ):
            has_font = any(
                'font: ${leo.font.' in lines[j]
                for j in range(max(0, i - 12), i)
            )
            if has_font:
                i += 1
                continue
        out.append(line)
        i += 1
    return '\n'.join(out)


def remove_trailing_font_props_after_shorthand(content: str) -> str:
    return re.sub(
        r'(font: \$\{leo\.font\.[^}]+\};)\s*'
        r'(?:font-style:\s*normal;\s*)?'
        r'(?:font-weight:\s*\d+;\s*)?'
        r'(?:line-height:\s*[^;]+;\s*)?',
        r'\1\n',
        content,
    )


def process(path: Path) -> bool:
    orig = path.read_text()
    content = orig
    content = fix_font_style_middle(content)
    content = remove_style_weight_before_font(content)
    content = cleanup_orphan_weights(content)
    content = fix_broken_indent(content)
    content = fix_responsive_font_size(content)
    content = remove_trailing_font_props_after_shorthand(content)
    content = remove_orphan_font_lines(content)
    if content != orig:
        path.write_text(content)
        return True
    return False


def main():
    n = 0
    for pattern in (
        '**/*.ts',
        '**/*.tsx',
    ):
        for path in ROOT.glob(pattern):
            if 'scripts' in path.parts:
                continue
            if process(path):
                n += 1
    print(f'Cleaned {n} files')


if __name__ == '__main__':
    main()

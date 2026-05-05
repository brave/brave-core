#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Fake plaster error report previewer.

Renders mock terminal output for every error type defined in
plaster_errors_spec.md. Run directly to iterate on the visual design:

    tools/cr/plaster_error_preview.py
"""
import io as _io_mod
from rich.console import Console
from rich.syntax import Syntax
from rich.text import Text

console = Console()

# ---------------------------------------------------------------------------
# Fake TOML content used as the "plaster file" for most examples.
# Line numbers here are real — renders reference specific line_range slices.
# ---------------------------------------------------------------------------

# Broken TOML for the P001 example — has an unquoted value on line 9.
# tomllib raises: "Invalid value (at line 9, column 11)"
BROKEN_TOML = """\
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

[[substitution]]
description = 'Moving CookieMonster into chromium_impl'
replace = namespace_net_chromium_impl
count = 2
"""

COOKIE_MONSTER_TOML = """\
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

[[substitution]]
description = 'Moving CookieMonster into chromium_impl'
pattern = 'namespace net'
replace = 'namespace net::chromium_impl'
count = 2

[[substitution]]
pattern = 'namespace net'
replace = 'namespace net::chromium_impl'

[[substitution]]
description = 'Replace class name'
pattern = 'CookieMonster'
re_pattern = 'Cookie\\w+'
replace = 'BraveCookieMonster'

[[substitution]]
description = 'Some substitution'
re_pattern = 'Cookie\\w+'
re_flags = ['MULTILINE', 'x']
replace = 'BraveCookie'

[[substitution]]
description = 'Fix function call'
re_pattern = 'void Foo((?unclosed'
replace = 'void BraveBar('

[[substitution]]
description = 'Moving CookieMonster into chromium_impl'
pattern = 'namespace net'
replace = 'namespace net::chromium_impl'

[[substitution]]
description = 'Moving CookieMonster into chromium_impl'
pattern = 'namespace net'
replace = 'namespace net::chromium_impl'
count = 0

[[substitution]]
description = 'Moving CookieMonster into chromium_impl'
pattern = 'namespace net'
replace = 'namespace net::chromium_impl'
count = -1

[[substitution]]
description = 'Replace namespace'
re_pattern = 'namespace net'
replace = 'namespace \\1::chromium_impl'
count = 1

[[substitution]]
description = 'Replace namespace'
pattern = 'namespace net'
re_flags = ['MULTILINE']
replace = 'namespace net::chromium_impl'
count = 1
"""

# TOML for W001/W002 warning examples.
# Line 7:  long description (> 80 cols) — triggers W001.
# Lines 13-14: multi-line description without blank separator — triggers W002.
FORMAT_TOML = """\
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

[[substitution]]
description = 'Moving CookieMonster into the net::chromium_impl namespace to prevent conflicts.'
pattern = 'namespace net'
replace = 'namespace net::chromium_impl'
count = 1

[[substitution]]
description = '''Moves CookieMonster into chromium_impl.
This also updates all forward declarations.'''
pattern = 'class CookieMonster'
replace = 'class BraveCookieMonster'
"""

# Fake C++ source for the P030/P031 source-context block.
# Three occurrences of 'namespace net' at lines 8, 15, 19.
# Each match has three natural context lines immediately above it.
COOKIE_MONSTER_CC = """\
// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cookies/cookie_monster.h"
#include "net/cookies/cookie_store.h"
#include "base/logging.h"
namespace net {

class CookieMonster : public CookieStore {
 public:
  void AddCookie(const CanonicalCookie& cc);
  void DeleteCookie(const GURL& url, const std::string& name);
};
namespace net {
  // Internal helper namespace — avoid this pattern.
  CookieMonster* g_instance = nullptr;
}
} // namespace net
"""

# ---------------------------------------------------------------------------
# Rendering helpers
# ---------------------------------------------------------------------------

_pipe_col_cache: dict[str, int] = {}


def _pipe_col(content: str) -> int:
    """Column index of the │ separator in rich.Syntax line-number output.

    Uses a force_terminal console so │ is always rendered, and Text.from_ansi
    for ANSI parsing — the same code-path as Text.stylize() — so the measured
    position is guaranteed to match the actual character positions.
    """
    if content in _pipe_col_cache:
        return _pipe_col_cache[content]
    tmp = Console(file=_io_mod.StringIO(), force_terminal=True,
                  force_jupyter=False, width=console.width,
                  color_system="truecolor")
    s = Syntax(content, "toml", line_numbers=True, line_range=(1, 1))
    with tmp.capture() as cap:
        tmp.print(s, end="")
    first = Text.from_ansi(cap.get()).plain.split("\n")[0]
    col = first.index("│") if "│" in first else len(str(len(content.splitlines()))) + 2
    _pipe_col_cache[content] = col
    return col


def _syn(content: str, line_range: tuple[int, int]) -> Syntax:
    return Syntax(content, "toml", line_numbers=True, line_range=line_range)


def _ann(content: str, leading_code: str, span_len: int, message: str,
         style: str = "bold red", bg: str = "on grey19",
         char: str = "^") -> Text:
    """Annotation line: spaces | <leading_code spaces><markers> message.

    leading_code: code text before the annotated region (sets marker offset).
    span_len:     number of marker characters.
    char:         "^" for a single-point annotation; "~" for an offending range.
    bg:           background style applied to the full line and padded to
                  console.width so the row contrasts visually with the Syntax
                  segments above and below it. Pass "" to disable.
    """
    col = _pipe_col(content)
    gutter = col + 1  # past │
    bg_suf = f" {bg}" if bg else ""
    t = Text()
    t.append(" " * (gutter + len(leading_code)), style=bg or "")
    t.append(char * span_len + "  " + message, style=f"{style}{bg_suf}")
    if bg:
        visible = gutter + len(leading_code) + span_len + 2 + len(message)
        pad = max(0, console.width - visible)
        if pad:
            t.append(" " * pad, style=bg)
    return t


def _hdr(code: str, message: str, level: str = "error") -> Text:
    t = Text()
    if level == "warning":
        t.append("warning", style="bold yellow")
        t.append(f"[{code}]", style="yellow")
    else:
        t.append("error", style="bold red")
        t.append(f"[{code}]", style="red")
    t.append(f": {message}")
    return t


def _gutter_bar() -> Text:
    return Text("   |", style="bold blue")


def _note(text: str) -> Text:
    t = Text()
    t.append("   = note: ", style="bold green")
    t.append(text)
    return t


def _hint(text: str) -> Text:
    t = Text()
    t.append("   = hint: ", style="bold cyan")
    t.append(text)
    return t


def _hint_cont(text: str) -> Text:
    return Text("           " + text)


def _src_ptr(path: str, label: str = "") -> Text:
    t = Text()
    t.append("  ──▶ ", style="bold blue")
    t.append(path)
    if label:
        t.append(f" ({label})", style="dim")
    return t


def _toml_match_line(content: str, lineno: int, leading_code: str, match_len: int,
                     level: str = "error") -> Text:
    """TOML line with Syntax token colours + background overlay on the annotated range.

    Uses the same Text.from_ansi() round-trip as _src_match_line so the gutter
    is pixel-perfect with the surrounding _syn() blocks.  level controls colour:
    'error' → dark_red, 'warning' → dark_goldenrod.
    """
    tmp = Console(file=_io_mod.StringIO(), force_terminal=True,
                  force_jupyter=False, width=console.width,
                  color_system="truecolor")
    with tmp.capture() as cap:
        tmp.print(Syntax(content, "toml", line_numbers=True,
                         line_range=(lineno, lineno)), end="")
    raw = cap.get()
    colored = Text.from_ansi(raw.rstrip("\n"))
    bg_style = "bold on dark_goldenrod" if level == "warning" else "bold on dark_red"
    plain = colored.plain
    if "│" in plain:
        gutter_width = plain.index("│") + 1
    else:
        gutter_width = len(str(len(content.splitlines()))) + 3
    offset = gutter_width + len(leading_code)
    colored.stylize(bg_style, offset, offset + match_len)
    return colored


def _src_syn(source: str, line_range: tuple[int, int], lang: str = "cpp") -> Syntax:
    """Syntax block for a Chromium source file slice.

    lang is derived from the plaster file's target extension at call time;
    defaults to 'cpp' for the .cc/.h examples in this preview.
    """
    return Syntax(source, lang, line_numbers=True, line_range=line_range)


def _src_match_line(source: str, lineno: int,
                    match_start: int, match_len: int,
                    excess: bool = False, lang: str = "cpp") -> Text:
    """Source match line with Syntax token colours + match region highlighted.

    Renders the full line (gutter + content) through Syntax with line_numbers=True
    so the gutter is pixel-perfect with the surrounding _src_syn() context blocks.
    The match span is then overlaid with the green/red background via Text.stylize().
    """
    tmp = Console(file=_io_mod.StringIO(), force_terminal=True,
                  force_jupyter=False, width=console.width,
                  color_system="truecolor")
    with tmp.capture() as cap:
        tmp.print(Syntax(source, lang, line_numbers=True,
                         line_range=(lineno, lineno)), end="")
    raw = cap.get()
    colored = Text.from_ansi(raw.rstrip("\n"))
    bg_style = "bold on dark_red" if excess else "bold on dark_green"
    # Measure from colored.plain — same ANSI-parsing code-path as stylize(),
    # so the │ index is guaranteed to match the Text's character positions.
    plain = colored.plain
    if "│" in plain:
        gutter_width = plain.index("│") + 1  # past │; +1 not +2 (space counts visually)
    else:
        gutter_width = len(str(len(source.splitlines()))) + 3
    colored.stylize(bg_style,
                    gutter_width + match_start,
                    gutter_width + match_start + match_len)
    return colored


def _src_ann(source: str, match_start: int, match_len: int, message: str,
             excess: bool = False, bg: str = "on grey19") -> Text:
    """Tilde underline for a source match, aligned to the Syntax gutter.

    match_start: 0-based column of the first matched character on the line.
    match_len:   number of ~ characters (= length of the matched text).
    excess:      True → red (unexpected), False → green (expected).
    """
    col = _pipe_col(source)
    gutter = col + 1  # past │; +1 not +2 (space counts visually)
    style = "bold red" if excess else "bold green"
    bg_suf = f" {bg}" if bg else ""
    t = Text()
    t.append(" " * (gutter + match_start), style=bg or "")
    t.append("~" * match_len + "  " + message, style=f"{style}{bg_suf}")
    if bg:
        visible = gutter + match_start + match_len + 2 + len(message)
        pad = max(0, console.width - visible)
        if pad:
            t.append(" " * pad, style=bg)
    return t


def _toml_annotated(content: str, lineno: int, leading_code: str, match_len: int,
                    message: str, level: str = "error") -> tuple[Text, Text]:
    """Paired TOML match line + tilde annotation — always rendered together.

    level controls colour: 'error' → red, 'warning' → yellow.
    """
    style = "bold red" if level == "error" else "bold yellow"
    return (
        _toml_match_line(content, lineno, leading_code, match_len, level),
        _ann(content, leading_code, match_len, message, style=style, char="~"),
    )


def _src_annotated(source: str, lineno: int, match_start: int, match_len: int,
                   message: str, excess: bool = False) -> tuple[Text, Text]:
    """Paired source match line + tilde annotation — always rendered together.

    excess=False → green (expected match), excess=True → red (unexpected match).
    """
    return (
        _src_match_line(source, lineno, match_start, match_len, excess=excess),
        _src_ann(source, match_start, match_len, message, excess=excess),
    )


def _ellipsis() -> Text:
    return Text("    ...", style="dim")


def _box_open(path: str, hint: str = "") -> Text:
    t = Text()
    t.append("╭───▶ ", style="bold blue")
    t.append(path)
    if hint:
        t.append(f" ({hint})", style="dim")
    return t


def _box_close() -> Text:
    return Text("╰───", style="bold blue")


def _box_bar() -> Text:
    return Text("|", style="bold blue")


def _prefixed(t: Text) -> Text:
    result = Text()
    result.append("| ", style="bold blue")
    result.append_text(t)
    return result


def _prefixed_syn(syn: Syntax) -> Text:
    tmp = Console(file=_io_mod.StringIO(), force_terminal=True,
                  force_jupyter=False, width=console.width,
                  color_system="truecolor")
    with tmp.capture() as cap:
        tmp.print(syn, end="")
    lines = cap.get().rstrip("\n").split("\n")
    t = Text()
    for i, line in enumerate(lines):
        if i > 0:
            t.append("\n")
        t.append("| ", style="bold blue")
        t.append_text(Text.from_ansi(line))
    return t


def _sep():
    console.print()


def _divider(label: str):
    console.print()
    console.rule(f"[dim]{label}[/dim]", style="dim")
    console.print()


# ---------------------------------------------------------------------------
# One render function per error code
# ---------------------------------------------------------------------------

def render_P002():
    console.print(_hdr("P002", "plaster file contains no substitutions"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml"))
    console.print(_box_bar())
    console.print(_prefixed(_note("A plaster file must define at least one [[substitution]] table.")))
    console.print(_prefixed(_hint("Add a substitution or delete this file if it is no longer needed.")))
    console.print(_box_close())


def render_P001():
    # Simulate the parse error position extracted from TOMLDecodeError message.
    # BROKEN_TOML has `replace = namespace_net_chromium_impl` on line 8.
    # tomllib would raise: "Invalid value (at line 8, column 11)"
    error_line = 8
    error_col = 11   # 1-based column; `replace = ` is 10 chars, value at col 11
    toml_msg = "Invalid value (at line 8, column 11)"

    c = BROKEN_TOML
    console.print(_hdr("P001", "malformed plaster file"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml"))
    console.print(_box_bar())
    # Context: up to and including the error line
    context_start = max(1, error_line - 3)
    console.print(_prefixed_syn(_syn(c, (context_start, error_line))))
    # Annotation: ^ at the reported column. error_col is 1-based; the extra +1
    # absorbs the space that Rich prints between │ and content.
    leading = " " * error_col
    console.print(_prefixed(_ann(c, leading, 1, "invalid value")))
    # Lines after the error (a couple for context)
    total = len(c.splitlines())
    if error_line < total:
        console.print(_prefixed_syn(_syn(c, (error_line + 1, min(error_line + 2, total)))))
    console.print(_box_bar())
    console.print(_prefixed(_note(f"tomllib.TOMLDecodeError: {toml_msg}")))
    console.print(_prefixed(_hint("Validate with:")))
    console.print(_prefixed(_hint_cont(
        'python3 -c "import tomllib, pathlib; '
        "tomllib.loads(pathlib.Path('rewrite/...toml').read_text())\"")))
    console.print(_prefixed(_hint_cont("Or paste the file into an online parser:")))
    console.print(_prefixed(_hint_cont("  https://codeshack.io/toml-formatter-validator/")))
    console.print(_box_close())


def render_P010():
    c = COOKIE_MONSTER_TOML
    console.print(
        _hdr("P010", "substitution #2 is missing a required 'description' field"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #2"))
    console.print(_box_bar())
    console.print(_prefixed_syn(_syn(c, (12, 14))))
    console.print(_box_bar())
    console.print(_prefixed(_note("Every substitution must include a 'description' explaining its intent.")))
    console.print(_prefixed(_hint("'description' must be the first field after '[[substitution]]':")))
    console.print(_prefixed(_hint_cont("  [[substitution]]")))
    console.print(_prefixed(_hint_cont("  description = '<one-line explanation>'")))
    console.print(_prefixed(_hint_cont("  pattern     = ...   # other fields follow")))
    console.print(_box_close())


def render_P011():
    c = COOKIE_MONSTER_TOML
    console.print(_hdr("P011", "substitution #1 specifies no pattern to match"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #1"))
    console.print(_box_bar())
    # Show the whole substitution #1 block (lines 6-10) — pattern is absent
    console.print(_prefixed_syn(_syn(c, (6, 10))))
    console.print(_box_bar())
    console.print(
        _prefixed(_note("A substitution must have exactly one of 'pattern' or 're_pattern'.")))
    console.print(_prefixed(_hint("Add one of:")))
    console.print(_prefixed(_hint_cont("  pattern = '<literal string>'")))
    console.print(_prefixed(_hint_cont("  re_pattern = '<python regex>'")))
    console.print(_box_close())


def render_P012():
    c = COOKIE_MONSTER_TOML
    console.print(_hdr("P012", "substitution #1 specifies no replacement text"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #1"))
    console.print(_box_bar())
    console.print(_prefixed_syn(_syn(c, (6, 9))))
    console.print(_box_bar())
    console.print(_prefixed(_hint("Add: replace = '<replacement string>'")))
    console.print(_prefixed(_hint_cont(r"  Use '\1', '\2', etc. for capture group back-references.")))
    console.print(_box_close())


def render_P013():
    c = COOKIE_MONSTER_TOML
    console.print(
        _hdr("P013", "substitution #3 specifies both 'pattern' and 're_pattern'"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #3"))
    console.print(_box_bar())
    # Segment A: [[substitution]] header + description, then highlighted pattern line
    console.print(_prefixed_syn(_syn(c, (16, 17))))
    for line in _toml_annotated(c, 18, "pattern = ", len("'CookieMonster'"),
                                 "remove one of these two"):
        console.print(_prefixed(line))
    # Segment B: highlighted re_pattern line
    for line in _toml_annotated(c, 19, "re_pattern = ", len("'Cookie\\w+'"), "or this one"):
        console.print(_prefixed(line))
    # Segment C: replace line
    console.print(_prefixed_syn(_syn(c, (20, 20))))
    console.print(_box_bar())
    console.print(_prefixed(_note("Only one match method may be used per substitution.")))
    console.print(_prefixed(_hint("Use 'pattern' for a plain literal match.")))
    console.print(_prefixed(_hint_cont("Use 're_pattern' when you need wildcards, groups, or flags.")))
    console.print(_box_close())


def render_P014():
    c = COOKIE_MONSTER_TOML
    console.print(_hdr("P014", "unknown regex flag 'x' in substitution #4"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #4"))
    console.print(_box_bar())
    # Segment A: [[substitution]] through re_pattern line, then highlighted re_flags line
    console.print(_prefixed_syn(_syn(c, (22, 24))))
    for line in _toml_annotated(c, 25, "re_flags = ['MULTILINE', ", len("'x'"),
                                 "not a valid Python re flag"):
        console.print(_prefixed(line))
    # Segment B: replace line
    console.print(_prefixed_syn(_syn(c, (26, 26))))
    console.print(_box_bar())
    console.print(
        _prefixed(_note("Valid flags: DOTALL, IGNORECASE, MULTILINE, VERBOSE, ASCII, LOCALE, UNICODE.")))
    console.print(_prefixed(_hint("Remove 'x' or replace it with a valid flag name")))
    console.print(_prefixed(_hint_cont("(e.g. re.VERBOSE corresponds to 'VERBOSE').")))
    console.print(_box_close())


def render_P020():
    c = COOKIE_MONSTER_TOML
    # re_pattern = 'void Foo((?unclosed'
    # Error: unterminated subpattern. The second ( at position 9 starts the bad group.
    # Pattern chars: v(0)o(1)i(2)d(3) (4)F(5)o(6)o(7)((8)((9)?...
    re_error_pos = 9
    console.print(_hdr("P020", "invalid regular expression in substitution #5"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #5"))
    console.print(_box_bar())
    # Segment A: [[substitution]] through re_pattern line
    console.print(_prefixed_syn(_syn(c, (28, 30))))
    # ^ at re_error_pos within the pattern value
    leading = "re_pattern = '" + "void Foo((?unclosed"[:re_error_pos]
    console.print(
        _prefixed(_ann(c, leading, 1, f"position {re_error_pos}: unterminated subpattern")))
    # Segment B: replace line
    console.print(_prefixed_syn(_syn(c, (31, 31))))
    console.print(_box_bar())
    console.print(
        _prefixed(_note(f"re.error: missing ), unterminated subpattern at position {re_error_pos}")))
    console.print(
        _prefixed(_hint("Check for unbalanced parentheses. Test the pattern at regex101.com")))
    console.print(_prefixed(_hint_cont("before running plaster.")))
    console.print(_box_close())


def render_P030():
    c = COOKIE_MONSTER_TOML
    console.print(_hdr("P030", "pattern not found in source"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #1"))
    console.print(_box_bar())
    console.print(_prefixed_syn(_syn(c, (6, 7))))
    for line in _toml_annotated(c, 8, "pattern = ", len("'namespace net'"),
                                 "found 0"):
        console.print(_prefixed(line))
    console.print(_prefixed_syn(_syn(c, (9, 10))))
    console.print(_box_bar())
    console.print(
        _prefixed(_note("'namespace net' was not found anywhere in the source.")))
    console.print(
        _prefixed(_note("The upstream file may have changed since this plaster file was written.")))
    console.print(_prefixed(_hint("- Search the source:")))
    console.print(_prefixed(_hint_cont(
        "    grep -n 'namespace net' ../../net/cookies/cookie_monster.cc")))
    console.print(_prefixed(_hint_cont("- If the text changed upstream, update 'pattern' to match.")))
    console.print(_prefixed(_hint_cont("- If the symbol was removed, delete this substitution.")))
    console.print(_box_close())


def render_P031():
    c = COOKIE_MONSTER_TOML
    src = COOKIE_MONSTER_CC
    matched = "namespace net"
    console.print(_hdr("P031", "too many matches (found 3, expected 1)"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #1"))
    console.print(_box_bar())
    console.print(_prefixed_syn(_syn(c, (6, 7))))
    for line in _toml_annotated(c, 8, "pattern = ", len("'namespace net'"),
                                 "3 matches found, expected 1"):
        console.print(_prefixed(line))
    console.print(_prefixed_syn(_syn(c, (9, 10))))
    console.print(_box_bar())
    console.print(
        _prefixed(_note("Use count = 3 to accept all matches, or count = 0 to skip validation.")))
    console.print(
        _prefixed(_hint("Narrow the pattern by including a unique neighbouring token,")))
    console.print(_prefixed(_hint_cont("or use 're_pattern' for a structural match.")))
    console.print(_box_bar())
    console.print(_prefixed(_src_ptr("net/cookies/cookie_monster.cc")))
    src_lines = src.splitlines()
    # Match 1 — expected (line 8): context lines 5-7 via Syntax, match line via Syntax
    console.print(_prefixed_syn(_src_syn(src, (5, 7))))
    for line in _src_annotated(src, 8, 0, len(matched), "(match 1 of 3 — expected)"):
        console.print(_prefixed(line))
    console.print(_prefixed(_ellipsis()))
    # Match 2 — unexpected (line 15): context lines 12-14 via Syntax, match line via Syntax
    console.print(_prefixed_syn(_src_syn(src, (12, 14))))
    for line in _src_annotated(src, 15, 0, len(matched), "(match 2 of 3 — unexpected)", excess=True):
        console.print(_prefixed(line))
    console.print(_prefixed(_ellipsis()))
    # Match 3 — unexpected (line 19): context lines 16-18 via Syntax, match line via Syntax
    line19 = src_lines[18]  # "} // namespace net"
    start = line19.index(matched)
    console.print(_prefixed_syn(_src_syn(src, (16, 18))))
    for line in _src_annotated(src, 19, start, len(matched), "(match 3 of 3 — unexpected)", excess=True):
        console.print(_prefixed(line))
    console.print(_box_close())


def render_P040():
    console.print(_hdr("P040", "source file does not exist"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml"))
    console.print(_box_bar())
    console.print(_prefixed(_note("Expected source at: ../../net/cookies/cookie_monster.cc")))
    console.print(_prefixed(_hint("- Verify the Chromium checkout is up to date (gclient sync).")))
    console.print(_prefixed(_hint_cont("- If the upstream file was renamed, update the plaster file path.")))
    console.print(_prefixed(_hint_cont("- If the upstream file was deleted, delete this plaster file too.")))
    console.print(_box_close())


def render_C001():
    console.print(_hdr("C001", "plaster file is out of date"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml"))
    console.print(_box_bar())
    console.print(_prefixed(_note(
        "The source or patch file would change if 'plaster apply' were run.")))
    console.print(_prefixed(_hint(
        "Run: plaster apply rewrite/net/cookies/cookie_monster.cc.toml")))
    console.print(_box_close())


def render_W001():
    c = FORMAT_TOML
    line7 = c.splitlines()[6]  # 0-indexed; line 7 (1-indexed) is the long description
    total_len = len(line7)
    overflow = total_len - 80
    console.print(_hdr("W001",
                        f"substitution #1 description exceeds 80 columns ({total_len} > 80)",
                        level="warning"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #1"))
    console.print(_box_bar())
    console.print(_prefixed_syn(_syn(c, (6, 6))))
    for line in _toml_annotated(c, 7, "", len("description"),
                                 f"{total_len} chars total", level="warning"):
        console.print(_prefixed(line))
    console.print(_prefixed_syn(_syn(c, (8, 10))))
    console.print(_box_bar())
    console.print(_prefixed(_note(f"Column 81+ overflow ({overflow} chars): `{line7[80:]}`")))
    console.print(_prefixed(_hint("Wrap long descriptions using a triple-quoted multi-line string:")))
    console.print(_prefixed(_hint_cont("  description = '''")))
    console.print(_prefixed(_hint_cont("  <one-line summary (under 80 chars)>")))
    console.print(_prefixed(_hint_cont("")))
    console.print(_prefixed(_hint_cont("  <optional body paragraph>")))
    console.print(_prefixed(_hint_cont("  '''")))
    console.print(_box_close())


def render_W002():
    c = FORMAT_TOML
    summary = "Moves CookieMonster into chromium_impl."
    console.print(_hdr("W002",
                        "substitution #2 description is missing a blank separator line",
                        level="warning"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #2"))
    console.print(_box_bar())
    # Segment A: [[substitution]] header, then highlighted first description line
    console.print(_prefixed_syn(_syn(c, (12, 12))))
    for line in _toml_annotated(c, 13, "description = '''", len(summary),
                                 "no blank line", level="warning"):
        console.print(_prefixed(line))
    # Segment B: body line + remaining fields
    console.print(_prefixed_syn(_syn(c, (14, 16))))
    console.print(_box_bar())
    console.print(_prefixed(_note("A description should follow Python docstring conventions:")))
    console.print(_prefixed(_hint("Add a blank line between the summary and the body:")))
    console.print(_prefixed(_hint_cont("  description = '''")))
    console.print(_prefixed(_hint_cont(f"  {summary}")))
    console.print(_prefixed(_hint_cont("")))
    console.print(_prefixed(_hint_cont("  This also updates all forward declarations.")))
    console.print(_prefixed(_hint_cont("  '''")))
    console.print(_box_close())


def render_P015():
    c = COOKIE_MONSTER_TOML
    console.print(_hdr("P015", "substitution #6 is missing a required 'count' field"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #6"))
    console.print(_box_bar())
    console.print(_prefixed_syn(_syn(c, (33, 36))))
    console.print(_box_bar())
    console.print(_prefixed(_note("Every substitution must declare the expected match count.")))
    console.print(_prefixed(_hint("Add one of:")))
    console.print(_prefixed(_hint_cont("  count = N          # N matches expected (validated at apply time)")))
    console.print(_prefixed(_hint_cont("  count = 0  # reason: <why validation is skipped>")))
    console.print(_box_close())


def render_P032():
    c = COOKIE_MONSTER_TOML
    src = COOKIE_MONSTER_CC
    matched = "namespace net"
    console.print(_hdr("P032", "too few matches (found 1, expected 3)"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #1"))
    console.print(_box_bar())
    console.print(_prefixed_syn(_syn(c, (6, 7))))
    for line in _toml_annotated(c, 8, "pattern = ", len("'namespace net'"),
                                 "1 of 3 expected matches found"):
        console.print(_prefixed(line))
    console.print(_prefixed_syn(_syn(c, (9, 10))))
    console.print(_box_bar())
    console.print(
        _prefixed(_note("Use count = 1 to accept only this match, or count = 0 to skip validation.")))
    console.print(_prefixed(_hint("Search for the missing occurrences:")))
    console.print(_prefixed(_hint_cont(
        "    grep -n 'namespace net' ../../net/cookies/cookie_monster.cc")))
    console.print(_prefixed(_hint_cont("If the source changed upstream, update count to match the new total.")))
    console.print(_box_bar())
    console.print(_prefixed(_src_ptr("net/cookies/cookie_monster.cc")))
    # The one found match — expected (green)
    console.print(_prefixed_syn(_src_syn(src, (5, 7))))
    for line in _src_annotated(src, 8, 0, len(matched), "(match 1 of 1 found — 2 more expected)"):
        console.print(_prefixed(line))
    console.print(_box_close())


def render_W003():
    c = COOKIE_MONSTER_TOML
    console.print(_hdr("W003",
                        "substitution #7 uses count = 0 without a reason comment",
                        level="warning"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #7"))
    console.print(_box_bar())
    # Segment A: [[substitution]] through replace, then highlighted count line
    console.print(_prefixed_syn(_syn(c, (38, 41))))
    for line in _toml_annotated(c, 42, "count = ", len("0"),
                                 "add a reason comment", level="warning"):
        console.print(_prefixed(line))
    console.print(_box_bar())
    console.print(_prefixed(_note("count = 0 accepts any nonzero number of matches (P031 and P032 will not fire).")))
    console.print(_prefixed(_hint_cont("P030 still fires if the pattern is not found at all.")))
    console.print(_prefixed(_hint("Add a reason comment on the same line:")))
    console.print(_prefixed(_hint_cont("  count = 0  # reason: <why validation is skipped>")))
    console.print(_box_close())


def render_P016():
    c = COOKIE_MONSTER_TOML
    console.print(_hdr("P016", "invalid count value in substitution #8"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #8"))
    console.print(_box_bar())
    console.print(_prefixed_syn(_syn(c, (44, 47))))
    for line in _toml_annotated(c, 48, "count = ", len("-1"),
                                 "not a valid count value"):
        console.print(_prefixed(line))
    console.print(_box_bar())
    console.print(_prefixed(_note("count must be an integer ≥ 0.")))
    console.print(_prefixed(_hint("Use count = N (N ≥ 1) for exact match validation,")))
    console.print(_prefixed(_hint_cont("or count = 0  # reason: <why> to accept any number.")))
    console.print(_box_close())


def render_P021():
    c = COOKIE_MONSTER_TOML
    # replace = 'namespace \1::chromium_impl' — \1 starts at position 10 in the value
    replace_val = "namespace \\1::chromium_impl"
    re_error_pos = 10
    leading = "replace = '" + replace_val[:re_error_pos]  # "replace = 'namespace "
    console.print(_hdr("P021", "invalid replacement string in substitution #9"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #9"))
    console.print(_box_bar())
    # Segment A: context lines before the replace line
    console.print(_prefixed_syn(_syn(c, (50, 52))))
    # Highlighted replace line: \1 is 2 chars at offset 21
    console.print(_prefixed(_toml_match_line(c, 53, leading, len("\\1"))))
    # ^ annotation at re_error_pos
    console.print(
        _prefixed(_ann(c, leading, 1,
                       f"position {re_error_pos}: invalid group reference",
                       char="^")))
    # Segment B: count line
    console.print(_prefixed_syn(_syn(c, (54, 54))))
    console.print(_box_bar())
    console.print(
        _prefixed(_note(f"re.error: invalid group reference 1 at position {re_error_pos}")))
    console.print(_prefixed(_hint("The pattern has no capture groups. Either add one:")))
    console.print(_prefixed(_hint_cont("  re_pattern = '(namespace) net'")))
    console.print(_prefixed(_hint_cont("or remove the back-reference from replace.")))
    console.print(_box_close())


def render_W004():
    c = COOKIE_MONSTER_TOML
    console.print(_hdr("W004",
                        "substitution #10 specifies re_flags but uses a literal pattern",
                        level="warning"))
    console.print(_box_open("rewrite/net/cookies/cookie_monster.cc.toml", "substitution #10"))
    console.print(_box_bar())
    console.print(_prefixed_syn(_syn(c, (56, 58))))
    for line in _toml_annotated(c, 59, "re_flags = ", len("['MULTILINE']"),
                                 "flags have no effect on literal patterns",
                                 level="warning"):
        console.print(_prefixed(line))
    console.print(_prefixed_syn(_syn(c, (60, 61))))
    console.print(_box_bar())
    console.print(_prefixed(_note("'pattern' is converted to a regex via re.escape() before compilation.")))
    console.print(_prefixed(_hint_cont(
        "Most flags (MULTILINE, VERBOSE, DOTALL, etc.) have no effect on")))
    console.print(_prefixed(_hint_cont("escaped literals. Only IGNORECASE is meaningful.")))
    console.print(_prefixed(_hint("Use 're_pattern' if you need flag-sensitive matching,")))
    console.print(_prefixed(_hint_cont("or remove 're_flags' if the flags are unintentional.")))
    console.print(_box_close())


def render_summary(n_errors: int, n_files_with_errors: int, n_total: int,
                   n_warnings: int = 0):
    hdr = Text()
    hdr.append("Apply plaster ", style="bold")
    if n_errors or n_warnings:
        hdr.append("failed.", style="bold red")
    else:
        hdr.append("succeeded.", style="bold green")
    console.print(hdr)
    rows = []
    if n_errors:
        lbl = Text()
        lbl.append("error(s)", style="bold red")
        lbl.append(f" across {n_files_with_errors} file(s)")
        rows.append((lbl, n_errors))
    if n_warnings:
        lbl = Text()
        lbl.append("warning(s)", style="bold yellow")
        rows.append((lbl, n_warnings))
    if n_total:
        lbl = Text()
        lbl.append("Plaster files ")
        lbl.append("applied", style="bold green")
        rows.append((lbl, n_total))

    label_w = max(len(lbl.plain) for lbl, _ in rows)
    num_w = max(len(str(n)) for _, n in rows)

    for lbl, n in rows:
        t = Text()
        t.append_text(lbl)
        t.append(" " * (label_w - len(lbl.plain) + 2))
        t.append(str(n).rjust(num_w), style="bold")
        console.print(t)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

_RENDERS = [
    ("P001 — toml-parse-error", render_P001),
    ("P002 — no-substitutions", render_P002),
    ("P010 — missing-description", render_P010),
    ("P011 — missing-pattern", render_P011),
    ("P012 — missing-replace", render_P012),
    ("P015 — missing-count", render_P015),
    ("P016 — invalid-count", render_P016),
    ("P013 — conflicting-patterns", render_P013),
    ("P014 — invalid-re-flags", render_P014),
    ("P020 — invalid-regex", render_P020),
    ("P021 — invalid-replace", render_P021),
    ("P030 — no-matches", render_P030),
    ("P031 — too-many-matches", render_P031),
    ("P032 — too-few-matches", render_P032),
    ("P040 — source-not-found", render_P040),
    ("C001 — plaster-out-of-date", render_C001),
    ("W001 — line-too-long", render_W001),
    ("W002 — description-format", render_W002),
    ("W003 — count-zero-no-reason", render_W003),
    ("W004 — flags-on-literal", render_W004),
]

if __name__ == "__main__":
    for label, fn in _RENDERS:
        _divider(label)
        fn()
    _sep()
    console.rule(style="dim")
    _sep()
    render_summary(n_errors=10, n_files_with_errors=3, n_total=13, n_warnings=2)
    _sep()

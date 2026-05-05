# Plaster Error Reporting Specification

## Behavioural expectations

### 1. Think before coding

Before writing any code, state your assumptions explicitly. If multiple
interpretations of the spec exist, present them — don't pick one silently.
If a simpler approach exists, say so and push back. If something is unclear,
stop, name what is confusing, and ask.

### 2. Simplicity first

Write the minimum code that solves the problem. Nothing speculative:

- No features beyond what the spec asks for.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that was not requested.
- No error handling for scenarios that cannot happen.
- If you write 200 lines and it could be 50, rewrite it.

Ask: *"Would a senior engineer say this is overcomplicated?"* If yes, simplify.

### 3. Surgical changes

Touch only what you must. When editing existing code: do not improve adjacent
code, comments, or formatting; do not refactor things that are not broken;
match the existing style even if you would do it differently; if you notice
unrelated dead code, mention it — do not delete it.

When your changes create orphans (unused imports, variables, functions),
remove them. Do not remove pre-existing dead code unless asked.

Every changed line should trace directly to a requirement in this spec.

### 4. Goal-driven execution

Transform each task into a verifiable goal before starting. For multi-step
work, state a brief plan:

```
1. [step] → verify: [check]
2. [step] → verify: [check]
```

Run the tests after each phase and confirm they pass before moving on.
Weak success criteria ("make it work") require constant clarification —
define what done looks like first.

### 5. British spelling

All prose, comments, docstrings, user-facing messages (error text, notes,
hints), and identifiers use British English spelling throughout — `colour`,
`behaviour`, `recognise`, `serialise`, `initialise`, etc. This applies to
every layer: implementation code, test code, and output strings the user
reads at the terminal. Exception: third-party API names that are spelled
in American English (e.g. `color_system`, `force_terminal`, `rich.Syntax`)
are kept exactly as-is — do not rename parameters or attributes owned by
an external library.

---

## Before starting — files to read

Read these files in full before planning or writing any code. They establish
the style, conventions, and APIs that all new code must follow.

| File | Purpose |
|------|---------|
| `tools/cr/plaster.py` | The implementation target. Read the full file to understand the existing structure before touching anything. |
| `tools/cr/plaster_error_preview.py` | Visual reference only. Run it (`python3 tools/cr/plaster_error_preview.py`) to see the expected terminal output for every error code. Do not copy its code into the implementation — it is a quick sketch written for iteration speed, not production quality. The live implementation must be written from scratch. |
| `tools/cr/plaster_test.py` | Test suite. Run after every change to confirm nothing regressed. |
| `docs/plaster.md` | User-facing documentation. Read for background on what plaster does and how plaster files are structured. |

---

## Goals

Plaster currently reports errors as raw strings flushed to stderr, with no
context, no source location, and no guidance for the user. This spec describes
a new error format designed after modern compiler output (Rust, TypeScript),
with the following properties:

- **Structured and scannable** — a human reading the output in a terminal can
  quickly identify *what* failed, *where*, and *why*.
- **Source-anchored** — errors point to both the plaster TOML (the rule that
  was misconfigured) and the Chromium source file (the target that was being
  modified), so there is no guesswork about which file to open.
- **Resilient** — a broken substitution should not abort the entire run. Plaster
  should collect all errors across all substitutions in a file and report them
  together, in the same way that a compiler reports all type errors before
  stopping. The *only* acceptable crash is an assertion violation (an internal
  invariant was broken).
- **Unicode-structured, color-annotated** — the structural decoration (`──▶`, `|`,
  `^`, `~`, `=`) uses stable Unicode characters so the format degrades cleanly
  to a log file or a CI transcript. ANSI color and background highlights are
  applied on top when the output is a real terminal; they can be stripped without
  losing meaning.

---

## Error Format Anatomy

A single error message is made up of three visual layers, top to bottom:

```
error[PXXX]: <short one-line description>
╭───▶ <plaster_file_path> (<location hint>)
|
| NN │ <toml line>
|       ~~~~~~~~~ <inline annotation, if needed>
| NN │ <toml line>
|
|    = note: <additional factual context>
|    = hint: <suggested corrective action>
|
|   ──▶ <chromium_source_file_path>
| NN | <source line>
|       ~~~~~~~~~~   <match annotation>
╰───
```

### Layer 1 — Error header and diagnostic box

The **error header** is always one line: `error[PXXX]: message`. It is
immediately followed by `╭───▶ <path>`, which opens the diagnostic box and
incorporates the TOML file pointer in the same line.

### Layer 2 — Substitution block

The **substitution block** is rendered using `rich.syntax.Syntax` with TOML
syntax highlighting (`language="toml"`) and `line_numbers=True`. The block
shows only the lines belonging to the relevant substitution entry.

The block does not have to be a single contiguous `Syntax` object. To place an
inline annotation immediately below a specific field line, the block is
**broken** at that line:

1. **Segment A** — `Syntax(content, "toml", line_numbers=True, line_range=(start, annotated_line - 1))`.
2. **Highlighted line** — the annotated line rendered via `_toml_match_line()`:
   a `Syntax(content, "toml", line_numbers=True, line_range=(annotated_line, annotated_line))`
   captured through ANSI, parsed with `Text.from_ansi()`, and overlaid with a
   background colour on the offending value span via `Text.stylize()`. Error
   lines use `bold on dark_red`; warning lines use `bold on dark_goldenrod`.
3. **Annotation** — a plain `rich.Text` line with `~` markers aligned under the
   highlighted value. Uses ASCII `~` for offending ranges or `^` for single
   points (see [Annotation marker style](#annotation-marker-style--vs-)).
4. **Segment B** — `Syntax(content, "toml", line_numbers=True, line_range=(annotated_line+1, end))`.

Segments A and B reference the same full source content string; only
`line_range` differs. This creates the visual of a continuous highlighted block
with an annotation woven in.

When multiple fields in the same substitution need annotations (e.g., P013),
the block is broken once per annotated field. A blank line is printed between
the last annotation and the `= note:` / `= hint:` lines.

Annotation line alignment:
- Prefix: spaces equal to the full Syntax gutter width (`│` column + 1).
  Use the actual rendered gutter width measured via `Text.from_ansi()` from a
  `force_terminal=True` capture — not a hand-calculated estimate — so the
  column matches exactly.
- No Syntax gutter. The annotation line has no Syntax line-number `│` gutter
  character — it carries only the box-side `| ` prefix that every line in the
  diagnostic box has, followed by spaces and the markers.
- Content: additional spaces equal to the length of the text before the
  annotated value, then the marker character (`^` or `~`) repeated for the
  value length.

### Layer 3 — Chromium source block (optional)

The **source block** is only shown for P031 and P032 (match count errors).
Like the substitution block, it uses `rich.syntax.Syntax` with
`line_numbers=True` for all lines — both context lines and match lines. The
file language (`"cpp"`, `"python"`, etc.) is derived from the source file
extension. `──▶` points to the Chromium source file.

Context lines are rendered as contiguous `Syntax` segments. Each match line is
rendered as a separate `Syntax(source, lang, line_numbers=True,
line_range=(lineno, lineno))` object — see the [Source match
highlight](#source-match-highlight) section for how the background colour is
overlaid without destroying the Syntax token colours.

Below each match line, a plain `rich.Text` annotation line uses `~` underlines
aligned to the match column, with no gutter character — only leading spaces.

### Annotation marker style: `^` vs `~`

Two annotation marker characters are used, and the choice is normative:

| Character | When to use                                                             |
|-----------|-------------------------------------------------------------------------|
| `^`       | A position emitted by an **external tool** (parser, regex engine). Always a single character wide. Use only for `TOMLDecodeError` column and `re.error.pos`. |
| `~`       | The underlined text **is** the problem — it could be selected and deleted or replaced to fix the issue. Repeat for the full span. Use for everything else. |

**Examples of `^`:** the column reported by `tomllib.TOMLDecodeError`; the
`.pos` index from `re.error`.

**Examples of `~`:** the `description` key in W001 (the key's full text
identifies which field to shorten); the full value of `pattern` in P013 (remove
this field); the full value of the invalid flag in P014 (replace this string);
the full matched text in source annotations (P031); the pattern value in
P030/P031 (the span of text that produced too many or too few matches); the
summary text in W002 (insert a blank line after this range).

Both `^` and `~` appear in **TOML annotation lines** (using `_ann()`) and in
**source annotation lines** (using `_src_ann()`, which always uses `~`). Neither
type of annotation line includes a `│` or `|` gutter character.

### Notes and hints

`= note:` provides factual context (e.g., the exact counts). `= hint:`
suggests a next action. Both are optional. They are plain `rich.Text` lines
printed below the substitution block.

---

## Colour Scheme

All colour output is produced through `rich` (already imported). The following
table is normative; the implementation must use these styles:

| Element                          | Rich style                      | Meaning                     |
|----------------------------------|---------------------------------|-----------------------------|
| `error` label                    | `bold red`                      | Fatal problem               |
| `warning` label                  | `bold yellow`                   | Requires attention          |
| Error / check code `[PXXX]`/`[CXXX]` | `red`                      | Lookup key                  |
| `╭───▶` box opener / TOML path   | `bold blue`                     | Opens diagnostic box        |
| `╰───` box closer                | `bold blue`                     | Closes diagnostic box       |
| `|` box sides / `──▶` src arrow  | `bold blue`                     | Source location             |
| Line numbers (Syntax gutter)     | dim (rendered by rich.Syntax)   | Navigation aid              |
| `^` underline (error)                 | `bold red`                      | Single-point error annotation       |
| `^` underline (warning)               | `bold yellow`                   | Single-point warning annotation     |
| `~` underline (offending range, error)| `bold red`                      | Ranged error annotation             |
| `~` underline (offending range, warn) | `bold yellow`                   | Ranged warning annotation           |
| `~` underline (expected match)        | `bold green`                    | Match that was expected             |
| `~` underline (unexpected match)      | `bold red`                      | Match beyond the expected count     |
| TOML syntax highlighting         | rendered by rich.Syntax (toml)  | Code readability            |
| `= note:` label                  | `bold green`                    | Factual context             |
| `= hint:` label                  | `bold cyan`                     | Suggested action            |
| Source code (default)            | `default`                       | Normal                      |
| Source code (match region)       | `bold on dark_green`            | A match that was found      |
| Source code (excess match)       | `bold on dark_red`              | An unexpected extra match   |
| Summary line `error:`            | `bold red`                      | Run conclusion              |
| Summary line `warning:`          | `bold yellow`                   | Run conclusion              |
| Summary count numbers            | `bold`                          | Emphasis                    |

**Colour is suppressed** (raw text only) when:
- stdout/stderr is not a TTY (pipe, file redirection).
- `--infra-mode` is active.
- The `NO_COLOR` environment variable is set.

`rich` handles TTY detection automatically; the implementation should not
check for it manually.

---

## Error Codes

| Code | Name                        | Category        |
|------|-----------------------------|-----------------|
| P001 | `toml-parse-error`          | File format     |
| P002 | `no-substitutions`          | File format     |
| P010 | `missing-description`       | Configuration   |
| P011 | `missing-pattern`           | Configuration   |
| P012 | `missing-replace`           | Configuration   |
| P015 | `missing-count`             | Configuration   |
| P016 | `invalid-count`             | Configuration   |
| P013 | `conflicting-patterns`      | Configuration   |
| P014 | `invalid-re-flags`          | Configuration   |
| P020 | `invalid-regex`             | Regex           |
| P021 | `invalid-replace`           | Regex           |
| P030 | `no-matches`                | Application     |
| P031 | `too-many-matches`          | Application     |
| P032 | `too-few-matches`           | Application     |
| P040 | `source-not-found`          | I/O             |

## Warning Codes

Warnings cause plaster to exit 1, the same as errors. The summary header shows
`Apply plaster failed.` whenever warnings are present, even if no errors were
raised.

| Code | Name                        | Category        |
|------|-----------------------------|-----------------|
| W001 | `line-too-long`             | Style           |
| W002 | `description-format`        | Style           |
| W003 | `count-zero-no-reason`      | Safety          |
| W004 | `flags-on-literal`          | Safety          |

## Check Codes

Check codes are only generated when plaster runs in `check` (dry-run) mode via
`Compiler.build(dry_run=True)`. They do not appear during `apply`.

| Code | Name                  | Category |
|------|-----------------------|----------|
| C001 | `plaster-out-of-date` | Check    |

---

## Error Catalogue and Examples

The examples below are annotated with `# <- color` comments to show what each
line looks like when colour output is active. These comments are not part of
the actual terminal output.

Lines beginning with `NN │` come from `rich.syntax.Syntax` (TOML or source
highlighted, colour applied by rich). This applies to both the substitution
block and the Chromium source block — all numbered code lines go through
`Syntax`. Annotation lines (with `^` or `~` markers) are plain `rich.Text`
lines inserted between Syntax segments; they contain no Syntax line-number `│`
gutter character — the box-side `| ` prefix is present, followed by leading
spaces that align the markers with the annotated value.

---

### P001 — `toml-parse-error`

Raised when `tomllib.loads()` throws `TOMLDecodeError`. The raw file text is
still available, so a context block **is** shown around the error line using
`rich.syntax.Syntax` (TOML highlighting, `line_numbers=True`). The block is
broken after the offending line to insert a `^` annotation at the column
reported by the parser. The full exception message goes in `= note:`.

Line and column are extracted from the `TOMLDecodeError` message with a regex:
`r"at line (\d+), col(?:umn)? (\d+)"`. When the position is found, show 3
context lines above the error line plus the error line itself:
`line_range=(max(1, error_line - 3), error_line)`. If no match, omit the
annotation and show a 5-line window starting from line 1.

```
error[P001]: malformed plaster file
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml
|
|   6 │
|   7 │ [[substitution]]
|   8 │ description = 'Moving CookieMonster into chromium_impl'
|   9 │ replace = namespace_net_chromium_impl
|                 ^  invalid value
|
|    = note: tomllib.TOMLDecodeError: Invalid value (at line 9, column 11)
|    = hint: Validate with:
|              python3 -c "import tomllib, pathlib; \
|                tomllib.loads(pathlib.Path('rewrite/...toml').read_text())"
|            Or paste the file into an online parser:
|            https://codeshack.io/toml-formatter-validator/
╰───
```

---

### P002 — `no-substitutions`

Raised when the TOML file parses successfully but contains no `[[substitution]]`
tables. A plaster file with no substitutions does nothing and is almost certainly
a mistake or a leftover after substitutions were removed. No substitution block
is shown because none exists.

```
error[P002]: plaster file contains no substitutions
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml
|
|    = note: A plaster file must define at least one [[substitution]] table.
|    = hint: Add a substitution or delete this file if it is no longer needed.
╰───
```

---

### P010 — `missing-description`

Raised when a `[[substitution]]` table has no `description` key. Show the
fields that *are* present so the user can see exactly what the substitution
looks like and where to add the missing key.

```
error[P010]: substitution #2 is missing a required 'description' field
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #2)
|
|  12 │ [[substitution]]                            # TOML syntax highlighted
|  13 │ pattern = 'namespace net'
|  14 │ replace = 'namespace net::chromium_impl'
|
|    = note: Every substitution must include a 'description' explaining its intent.
|    = hint: 'description' must be the first field after '[[substitution]]':
|              [[substitution]]
|              description = '<one-line explanation>'
|              pattern     = ...   # other fields follow
╰───
```

---

### P011 — `missing-pattern`

Raised when a substitution has neither `pattern` nor `re_pattern`. Show the
substitution header and any fields that are present.

```
error[P011]: substitution #1 specifies no pattern to match
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #1)
|
|   7 │ [[substitution]]
|   8 │ description = 'Moving CookieMonster into chromium_impl'
|   9 │ replace = 'namespace net::chromium_impl'
|
|    = note: A substitution must have exactly one of 'pattern' or 're_pattern'.
|    = hint: Add one of:
|              pattern = '<literal string>'
|              re_pattern = '<python regex>'
╰───
```

---

### P012 — `missing-replace`

Raised when a substitution has no `replace` key.

```
error[P012]: substitution #1 specifies no replacement text
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #1)
|
|   7 │ [[substitution]]
|   8 │ description = 'Moving CookieMonster into chromium_impl'
|   9 │ pattern = 'namespace net'
|
|    = hint: Add: replace = '<replacement string>'
|              Use '\1', '\2', etc. for capture group back-references.
╰───
```

---

### P015 — `missing-count`

Raised when a substitution has no `count` key. `count` is required so that
plaster can verify the number of matches and fail loudly if the source has
changed. Omitting it silently disables that check.

```
error[P015]: substitution #1 is missing a required 'count' field
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #1)
|
|   7 │ [[substitution]]
|   8 │ description = 'Moving CookieMonster into chromium_impl'
|   9 │ pattern = 'namespace net'
|  10 │ replace = 'namespace net::chromium_impl'
|
|    = note: Every substitution must declare the expected match count.
|    = hint: Add one of:
|              count = N          # N matches expected (validated at apply time)
|              count = 0  # reason: <why any number of matches is acceptable>
╰───
```

---

### P016 — `invalid-count`

Raised when a `count` key is present but its value is not a non-negative integer.
Valid values are integers ≥ 0. The `~` annotation underlines the offending value.
The block is broken after the `count` line to insert the annotation.

```
error[P016]: invalid count value in substitution #8
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #8)
|
|  44 │ [[substitution]]
|  45 │ description = 'Moving CookieMonster into chromium_impl'
|  46 │ pattern = 'namespace net'
|  47 │ replace = 'namespace net::chromium_impl'
|  48 │ count = -1                              # bold on dark_red bg on '-1'
|              ~~ not a valid count value       # bold red, ~ = offending range
|
|    = note: count must be an integer ≥ 0.
|    = hint: Use count = N (N ≥ 1) for exact match validation,
|            or count = 0  # reason: <why> to accept any number.
╰───
```

---

### P013 — `conflicting-patterns`

Raised when both `pattern` and `re_pattern` are present. Both offending fields
are annotated, requiring the substitution block to be **broken** twice: once
after the `pattern` line and once after the `re_pattern` line.

```
error[P013]: substitution #2 specifies both 'pattern' and 're_pattern'
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #2)
|
|  12 │ [[substitution]]
|  13 │ description = 'Replace class name'
|  14 │ pattern = 'CookieMonster'           # bold on dark_red bg on 'CookieMonster'
|                ~~~~~~~~~~~~~~~ remove one of these two    # bold red, ~ = offending range
|  15 │ re_pattern = 'Cookie\w+'            # bold on dark_red bg on 'Cookie\w+'
|                  ~~~~~~~~~~~ or this one  # bold red, ~ = offending range
|  16 │ replace = 'BraveCookieMonster'
|
|    = note: Only one match method may be used per substitution.
|    = hint: Use 'pattern' for a plain literal match.
|            Use 're_pattern' when you need wildcards, groups, or flags.
╰───
```

---

### P014 — `invalid-re-flags`

Raised when an entry in `re_flags` is not a valid Python `re` flag name. The
`~` underlines the full offending flag value within the array. The block is
broken after the `re_flags` line to insert the annotation.

```
error[P014]: unknown regex flag 'x' in substitution #3
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #3)
|
|  15 │ [[substitution]]
|  16 │ description = 'Some substitution'
|  17 │ re_pattern = 'Cookie\w+'
|  18 │ re_flags = ['MULTILINE', 'x']   # bold on dark_red bg on 'x'
|                               ~~~  not a valid Python re flag    # bold red, ~ = offending range
|  19 │ replace = 'BraveCookie'
|
|    = note: Valid flags are: DOTALL, IGNORECASE, MULTILINE, VERBOSE, ASCII,
|            LOCALE, UNICODE. Flag names must be upper-case strings.
|    = hint: Remove 'x' or replace it with a valid flag name
|            (e.g., re.VERBOSE corresponds to 'VERBOSE').
╰───
```

---

### P020 — `invalid-regex`

Raised when `re.compile()` raises `re.error`. The `re.error` exception exposes
`.pos` (0-based character index into the pattern string). The `^` caret is
placed at that position within the displayed value. The block is broken after
the `re_pattern` line.

Caret column formula: `gutter_width + len("re_pattern = '") + re_error.pos`.

```
error[P020]: invalid regular expression in substitution #3
╭───▶ rewrite/chrome/browser/autocomplete/autocomplete_classifier_factory.cc.toml (substitution #3)
|
|  18 │ [[substitution]]
|  19 │ description = 'Fix function call'
|  20 │ re_pattern = 'void Foo((?unclosed'
|                               ^            position 8: unterminated subpattern  # bold red
|  21 │ replace = 'void BraveBar('
|
|    = note: re.error: missing ), unterminated subpattern at position 8
|    = hint: Check for unbalanced parentheses. Test the pattern at regex101.com
|            before running plaster.
╰───
```

---

### P021 — `invalid-replace`

Raised when `re.subn()` raises `re.error` at apply time. This happens when the
`replace` string references a back-reference (e.g. `\1`) that does not exist in
the compiled pattern. P020 covers regex compilation failures; P021 covers
substitution failures.

`re.error` exposes `.pos` (0-based character index into the `replace` string
where the error was detected). The `^` caret is placed at that position within
the displayed value. The block is broken after the `replace` line.

Caret column formula: `gutter_width + len("replace = '") + re_error.pos`.

```
error[P021]: invalid replacement string in substitution #9
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #9)
|
|  50 │ [[substitution]]
|  51 │ description = 'Replace namespace'
|  52 │ re_pattern = 'namespace net'
|  53 │ replace = 'namespace \1::chromium_impl'   # bold on dark_red bg on '\1'
|                              ^  position 10: invalid group reference    # bold red
|  54 │ count = 1
|
|    = note: re.error: invalid group reference 1 at position 10
|    = hint: The pattern has no capture groups. Either add one:
|              re_pattern = '(namespace) net'
|            or remove the back-reference from replace.
╰───
```

---

### P030 — `no-matches`

Raised when `re.subn()` returns 0 matches, regardless of `count`. A
substitution that matches nothing is always an error — it does nothing to the
source, which is never the intent. The `~` annotation underlines the full value
of `pattern` or `re_pattern`. The block is broken after that line.

```
error[P030]: pattern not found in source
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #1)
|
|   7 │ [[substitution]]
|   8 │ description = 'Moving CookieMonster into chromium_impl'
|   9 │ pattern = 'namespace net'         # bold on dark_red bg on 'namespace net'
|                ~~~~~~~~~~~~~~~ found 0    # bold red, ~ = offending range
|  10 │ replace = 'namespace net::chromium_impl'
|
|    = note: The literal string 'namespace net' was not found anywhere in the source.
|            The upstream file may have changed since this plaster file was written.
|    = hint: - Search the source:
|                grep -n 'namespace net' ../../net/cookies/cookie_monster.cc
|            - If the text changed upstream, update 'pattern' to match.
|            - If the symbol was removed, delete this substitution.
╰───
```

---

### P031 — `too-many-matches`

Raised when `re.subn()` returns M matches but `count` is N and M > N. The
annotation underlines the pattern field. The source block shows all match
positions using at most 3 context lines around each match, separated by `...`
when match groups are more than 5 lines apart.

When colour is enabled:
- Expected matches (the first N) use `bold on dark_green` for the matched
  region within the source line.
- Excess matches use `bold on dark_red`.

The `~` underline (no-colour mode) spans the full matched text and is labelled
with the match index.

```
error[P031]: too many matches (found 3, expected 1)
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #1)
|
|   7 │ [[substitution]]
|   8 │ description = 'Moving CookieMonster into chromium_impl'
|   9 │ pattern = 'namespace net'         # bold on dark_red bg on 'namespace net'
|                ~~~~~~~~~~~~~~~ 3 matches found, expected 1    # bold red, ~ = offending range
|  10 │ replace = 'namespace net::chromium_impl'
|
|    = note: Use count = 3 to accept all matches, or count = 0 to accept any number.
|    = hint: If only one occurrence should be replaced, add surrounding context
|            to make the pattern more specific. Consider 're_pattern' for
|            structural matches that include a method name or block boundary.
|
|   ──▶ net/cookies/cookie_monster.cc
|   39 │ // some context                             # Syntax, 3 lines of context
|   40 │ // more context
|   41 │ // more context
|   42 │ namespace net {                             # Syntax, match line, bold on dark_green bg
|       ~~~~~~~~~~~~~~  (match 1 of 3 — expected)  # plain Text, ~ = bold green
|     ...
|  196 │ // context
|  197 │ // context
|  198 │ namespace net {                             # Syntax, bold on dark_red bg
|       ~~~~~~~~~~~~~~  (match 2 of 3 — unexpected)  # ~ = bold red
|     ...
|  310 │ // context
|  311 │ // context
|  312 │ } // namespace net                          # Syntax, bold on dark_red bg
|            ~~~~~~~~~~~~~~  (match 3 of 3 — unexpected)  # ~ = bold red, offset by 5 cols
╰───
```

---

### P032 — `too-few-matches`

Raised when `re.subn()` returns M matches, `count` is N, and `0 < M < N`. The
zero-match case is P030; this error covers partial matches only.

The annotation underlines the pattern field. The source block shows all M found
matches using the same context and `...` separator rules as P031. All found
matches are highlighted `bold on dark_green` — they are valid matches, just
fewer than declared. Each match line carries a `bold green` `~` annotation
labelled `(match X of M found — K more expected)` where K = N − M.

```
error[P032]: too few matches (found 1, expected 3)
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #1)
|
|   7 │ [[substitution]]
|   8 │ description = 'Moving CookieMonster into chromium_impl'
|   9 │ pattern = 'namespace net'         # bold on dark_red bg on 'namespace net'
|                ~~~~~~~~~~~~~~~ 1 of 3 expected matches found    # bold red, ~ = offending range
|  10 │ replace = 'namespace net::chromium_impl'
|
|    = note: Use count = 1 to accept only this match, or count = 0 to accept any number.
|    = hint: Search for the missing occurrences:
|                grep -n 'namespace net' ../../net/cookies/cookie_monster.cc
|            If the source changed upstream, update count to match the new total.
|
|   ──▶ net/cookies/cookie_monster.cc
|   39 │ // some context                             # Syntax, 3 lines of context
|   40 │ // more context
|   41 │ // more context
|   42 │ namespace net {                             # Syntax, match line, bold on dark_green bg
|       ~~~~~~~~~~~~~~  (match 1 of 1 found — 2 more expected)  # ~ = bold green
╰───
```

---

### P040 — `source-not-found`

Raised when the Chromium source file derived from the plaster file path does
not exist on disk. No substitution block is shown because no substitution has
been attempted yet.

```
error[P040]: source file does not exist
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml
|
|    = note: Expected source at: ../../net/cookies/cookie_monster.cc
|    = hint: - Verify the Chromium checkout is up to date (gclient sync).
|            - If the upstream file was renamed, update the plaster file path.
|            - If the upstream file was deleted, delete this plaster file too.
╰───
```

---

### C001 — `plaster-out-of-date`

Only emitted during `check` (dry-run) mode. Raised after all substitutions in a
file complete successfully but the resulting source or patch content differs from
what is currently on disk — meaning the file needs to be reapplied. No
substitution block is shown; the file header and a hint are sufficient.

```
error[C001]: plaster file is out of date
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml
|
|    = note: The source or patch file would change if 'plaster apply' were run.
|    = hint: Run: plaster apply rewrite/net/cookies/cookie_monster.cc.toml
╰───
```

---

### W001 — `line-too-long`

Raised when the `description` value in a `[[substitution]]` block exceeds 80
columns. The `~` annotation underlines the full `description` key to identify
the offending field. Because long lines are truncated in Syntax output, the
overflow text is shown explicitly in `= note:` rather than in the inline
annotation.

```
warning[W001]: substitution #1 description exceeds 80 columns (96 > 80)
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #1)
|
|   6 │ [[substitution]]
|   7 │ description = 'Moving CookieMonster into the net::chromium_impl namespace t
|       ~~~~~~~~~~~  96 chars total                                                 # bold yellow, ~ = offending range
|   8 │ pattern = 'namespace net'
|   9 │ replace = 'namespace net::chromium_impl'
|  10 │ count = 1
|
|    = note: Column 81+ overflow (16 chars): `vent conflicts.'`
|    = hint: Wrap long descriptions using a triple-quoted multi-line string:
|              description = '''
|              <one-line summary (under 80 chars)>
|
|              <optional body paragraph>
|              '''
╰───
```

---

### W002 — `description-format`

Raised when a multi-line `description` value does not follow the Python
docstring convention: a one-line summary on the first line, then a blank line,
then optional body paragraphs. The `~` underlines the full summary text to show
where the blank line should be inserted after it.

```
warning[W002]: substitution #2 description is missing a blank separator line
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #2)
|
|  12 │ [[substitution]]
|  13 │ description = '''Moves CookieMonster into chromium_impl.   # bold on dark_goldenrod bg on summary text
|                        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  no blank line  # bold yellow, ~ = offending range
|  14 │ This also updates all forward declarations.'''
|  15 │ pattern = 'class CookieMonster'
|  16 │ replace = 'class BraveCookieMonster'
|
|    = note: A description should follow Python docstring conventions:
|    = hint: Add a blank line between the summary and the body:
|              description = '''
|              Moves CookieMonster into chromium_impl.
|
|              This also updates all forward declarations.
|              '''
╰───
```

---

### W003 — `count-zero-no-reason`

Raised when a substitution has `count = 0` but the `count` line does not end
with a `# reason: <text>` comment. `count = 0` disables match-count validation
entirely; without a documented reason it is indistinguishable from an
accidental omission.

The `~` underlines the `0` value. The block is broken after the `count` line to
insert the annotation.

```
warning[W003]: substitution #1 uses count = 0 without a reason comment
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #1)
|
|   7 │ [[substitution]]
|   8 │ description = 'Moving CookieMonster into chromium_impl'
|   9 │ pattern = 'namespace net'
|  10 │ replace = 'namespace net::chromium_impl'
|  11 │ count = 0                               # bold on dark_goldenrod bg on '0'
|              ~  add a reason comment           # bold yellow, ~ = offending range
|
|    = note: count = 0 accepts any nonzero number of matches (P031 and P032 will not fire).
|            P030 still fires if the pattern is not found at all.
|    = hint: Add a reason comment on the same line:
|              count = 0  # reason: <why validation is skipped>
╰───
```

---

### W004 — `flags-on-literal`

Raised when a substitution specifies `re_flags` alongside `pattern` (not
`re_pattern`). The `pattern` value is run through `re.escape()` before regex
compilation, so most flags — `MULTILINE`, `VERBOSE`, `DOTALL`, etc. — have no
effect on the resulting pattern. Only `IGNORECASE` is meaningful on an escaped
literal. The `~` annotation underlines the full `re_flags` value. The block is
broken after the `re_flags` line to insert the annotation.

```
warning[W004]: substitution #10 specifies re_flags but uses a literal pattern
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #10)
|
|  56 │ [[substitution]]
|  57 │ description = 'Replace namespace'
|  58 │ pattern = 'namespace net'
|  59 │ re_flags = ['MULTILINE']               # bold on dark_goldenrod bg on '['MULTILINE']'
|               ~~~~~~~~~~~~~~~ flags have no effect on literal patterns    # bold yellow
|  60 │ replace = 'namespace net::chromium_impl'
|  61 │ count = 1
|
|    = note: 'pattern' is converted to a regex via re.escape() before compilation.
|            Most flags (MULTILINE, VERBOSE, DOTALL, etc.) have no effect on
|            escaped literals. Only IGNORECASE is meaningful.
|    = hint: Use 're_pattern' if you need flag-sensitive matching,
|            or remove 're_flags' if the flags are unintentional.
╰───
```

---

## Multiple Errors

Plaster must collect **all** errors within a single plaster file before
reporting them. Execution of subsequent substitutions continues even after a
failure in an earlier one (resilient mode). This matches how compilers report
all type errors in a file without stopping at the first one.

### File header line

Before the first diagnostic (error or warning) for a given `PlasterFile`,
print the command that would re-run plaster on that file alone:

```
plaster apply rewrite/net/cookies/cookie_monster.cc.toml
```

The command uses `plaster apply` in apply mode and `plaster check` in check mode.
This line is printed in **bold** so it stands out as a visual separator between
files. It is printed exactly once per file — not before every diagnostic — and
only when at least one diagnostic is produced. Files with no diagnostics produce
no output at all.

### Within a file

Within a single file, diagnostics are printed sequentially, separated by a
blank line:

```
plaster apply rewrite/net/cookies/cookie_monster.cc.toml

error[P031]: too many matches (found 2, expected 1)
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #1)
|   ...
╰───

error[P030]: pattern not found in source
╭───▶ rewrite/net/cookies/cookie_monster.cc.toml (substitution #2)
|   ...
╰───
```

### Across files

When processing multiple files, the bold file header provides the separation —
no extra blank line is inserted between the last diagnostic of one file and the
header of the next.

---

## Summary

After all plaster files have been processed, a multiline summary block is
printed. The first line is always `Apply plaster succeeded.` or
`Apply plaster failed.` — bold, so it reads clearly at the end of a long run.
Detail lines follow for each non-zero count; lines with a zero value are
omitted entirely:

When there are errors and warnings:

```
Apply plaster failed.
error(s) across 2 file(s)   3
warning(s)                   2
Plaster files applied            13
```

When there are only errors (no warnings):

```
Apply plaster failed.
error(s) across 2 file(s)   3
Plaster files applied            13
```

When there are only warnings (no errors):

```
Apply plaster failed.
warning(s)        2
Plaster files applied  13
```

When everything succeeds (no errors, no warnings):

```
Apply plaster succeeded.
Plaster files applied  13
```

Styling: the header line is `bold`; `failed` is `bold red`, `succeeded` is
`bold green`. Within each detail line only the key word is styled — `error(s)`
in `bold red`, `warning(s)` in `bold yellow`, `applied` in `bold green` —
the surrounding label text is plain and numbers are `bold`. Labels are
left-aligned and padded to the longest label in the current output plus two
spaces; numbers are right-aligned to each other in the column immediately
following.

In `check` mode, out-of-date files each produce a `C001` diagnostic (full box
format). The summary counts them as errors and uses `Apply plaster failed.`
accordingly.

---

## Execution Flow

### Why `terminal.with_status` must go

`apply()` currently wraps the **entire** processing loop in a single
`terminal.with_status()` call. `Terminal`'s spinner mechanism does additional
things beyond a simple status display that are not wanted here. Replace it with
`console.status()` directly, which gives full control over the label text,
scope, and teardown behaviour.

### Per-file progress: the ninja/siso model

While processing file `i` of `N`, display a transient spinner on one line:

```
⠸ (3/42) rewrite/net/cookies/cookie_monster.cc.toml
```

`Compiler` opens a single `console.status()` for the entire duration of
`Compiler.build()`. Before processing each file, it updates the spinner label
to reflect the current position:

```python
status.update(f'({i}/{n}) {label}')
```

Errors are rendered by calling `console.print()` while the spinner is still
active. `rich` handles this correctly: it erases the spinner line, prints the
output, then redraws the spinner below — so the spinner never conflicts with
error output and does not need to be stopped and restarted around each file.
The spinner stops once after all files are processed, immediately before the
summary is printed.

- **Clean run** — the spinner updates silently through all files and stops.
  No persistent output other than the summary.
- **Run with errors** — error blocks appear above the still-running spinner as
  each affected file is processed. The spinner continues to show progress for
  subsequent files.

### apply() loop

`apply(args)` constructs a `Compiler` and delegates the entire processing loop
to it:

```python
def apply(args):
    plaster_files = get_plaster_files(getattr(args, 'filepaths', None))
    return Compiler(console).build(plaster_files)
```

All spinner management, diagnostic collection, error rendering, and summary
output are handled by `Compiler` — see the [Architecture](#architecture) section
for the full implementation. Any diagnostic — error or warning — causes the run
to exit 1. Drop the per-file `console.log()` call — the `╭───▶` opener in each
error block already identifies the file, and a clean file produces no output.

### check() loop

`check()` performs a dry-run pass by passing `dry_run=True` to `Compiler.build`:

```python
def check(args):
    plaster_files = get_plaster_files(getattr(args, 'filepaths', None))
    return Compiler(console).build(plaster_files, dry_run=True)
```

When a dry run detects that a file would change, `PlasterFile` raises
`PlasterOutOfDateException` instead of writing. `Compiler` catches it and emits
a `C001` diagnostic for that file — same box format as all other diagnostics.

`check()` returns 0 only when no files are out of date and no validation errors
were found during the dry-run pass.

### Infra mode and non-TTY

In `--infra-mode`, or when stdout/stderr is not a TTY (pipe, CI log), suppress
the spinner entirely. `Compiler.build()` already handles this: it opens
`nullcontext()` instead of `console.status()` when `console.is_terminal` is
false, so no additional logic is needed in `apply()` or `check()`. Error blocks
and the summary still appear; only the spinner is omitted.

---

## Infra Mode (`--infra-mode`)

When `--infra-mode` is active, the error format must remain exactly the same
structure — no content is omitted. Colour is stripped automatically by `rich`
when the output is not a TTY. The `╭───▶` / `╰───` box borders, `|` sides,
`──▶` source arrows, `^` underlines, and `= note:` labels are all plain Unicode
and remain legible. `rich.syntax.Syntax`
degrades gracefully to plain numbered text without ANSI codes.

No additional CI-specific formatting (GitHub Actions annotations, escape codes
not produced by `rich`) should be added unless explicitly requested in a
separate spec.

---

## Resilience Contract

| Situation                                  | Behaviour                             |
|--------------------------------------------|---------------------------------------|
| TOML file is malformed                     | Report P001, skip this file           |
| No substitutions in file                   | Report P002, skip this file           |
| Required field missing                     | Report P010-P012, P015, continue next sub |
| Invalid count value                        | Report P016, skip this sub            |
| Conflicting fields                         | Report P013, skip this sub            |
| Invalid flag or regex                      | Report P014/P020, skip this sub       |
| Invalid replacement string                 | Report P021, skip this sub            |
| Match count wrong                          | Report P030/P031/P032, continue next sub |
| Source file missing                        | Report P040, skip this file           |
| File would change in dry-run mode          | Report C001, continue next file       |
| Unexpected Python exception in sub loop    | Re-raise (invariant violation)        |
| Assertion failure (`assert`)               | Crash -- invariant violated           |

The key principle: **errors from bad input are reported; errors from bugs in
plaster itself propagate normally**.

---

## Implementation Notes

### Code quality

The demo (`plaster_error_preview.py`) exists solely to pin down the visual
design quickly. It is a sketch: global state, inline constants, no error
handling, helper functions written for convenience rather than clarity. Do not
copy its code into the implementation. Do study it when facing a specific
visual problem — the demo often contains a working conceptual solution (e.g.
how it measures the gutter column, how it overlays a background colour on a
`Syntax`-highlighted line, how it aligns annotation markers). Extract the idea,
then write a clean version from scratch.

`IncendiaryErrorHandler` must be removed from `plaster.py`. The new structured
diagnostic pipeline handles all expected error conditions; unhandled exceptions
(genuine plaster bugs) should surface as plain Python tracebacks. No special
logging handler is required.

The live implementation in `plaster.py` must be written from scratch and held
to a much higher standard:

- Every rendering helper must have a clear, single responsibility and a
  descriptive name.
- No global mutable state; pass `console` and configuration explicitly.
- Helpers that compute layout (column widths, gutter offsets) must be
  documented with the formula they implement and why.
- No magic numbers — name every constant.
- The full diagnostic pipeline (collect → render → summarise) must be
  structured so that each stage can be tested independently.

### Architecture

#### Diagnostic data types

All diagnostic information is stored in frozen dataclasses so that diagnostic
objects are immutable values — safe to cache, compare, and pass freely.

Two small value types carry annotation data to the rendering layer:

```python
from typing import ClassVar

@dataclass(frozen=True)
class TomlAnnotation:
    line_no: int            # 1-based line in the plaster file
    col_start: int          # 0-based column of the annotated value start
    span_len: int           # number of marker characters
    message: str            # annotation label text
    char: str = "~"         # "~" for offending ranges; "^" for external tool positions

@dataclass(frozen=True)
class SourceAnnotation:
    line_no: int            # 1-based
    col_start: int          # 0-based column of match start on that line
    span_len: int           # length of matched text
    message: str            # e.g. "(match 1 of 3 — expected)"
    is_excess: bool = False # True → red (unexpected); False → green (expected)
```

`PlasterDiagnostic` is the shared base. `toml_annotations` holds every TOML
span to annotate — most codes produce one; P013 produces two. There is no
`code` instance field: each per-code subclass declares `CODE` as a `ClassVar`
constant, accessible as `d.CODE` but absent from `__init__`. `PlasterError` and
`PlasterWarning` are level discriminators used by `isinstance` checks in the
`Compiler` loop and in `render_diagnostic`.

```python
@dataclass(frozen=True)
class PlasterDiagnostic:
    message: str
    plaster_file: Path
    substitution_index: int | None           # 0-based; None for file-level
    toml_line_range: tuple[int, int] | None  # (first, last), 1-based
    toml_annotations: tuple[TomlAnnotation, ...] = ()
    notes: tuple[str, ...] = ()
    hints: tuple[str, ...] = ()

    def render(self, console: Console) -> None:
        raise NotImplementedError

@dataclass(frozen=True)
class PlasterError(PlasterDiagnostic): pass

@dataclass(frozen=True)
class PlasterWarning(PlasterDiagnostic): pass

# ── Per-code error classes ──────────────────────────────────────────────────

@dataclass(frozen=True)
class P001Error(PlasterError):
    CODE: ClassVar[str] = "P001"

@dataclass(frozen=True)
class P002Error(PlasterError):
    CODE: ClassVar[str] = "P002"

@dataclass(frozen=True)
class P010Error(PlasterError):
    CODE: ClassVar[str] = "P010"

@dataclass(frozen=True)
class P011Error(PlasterError):
    CODE: ClassVar[str] = "P011"

@dataclass(frozen=True)
class P012Error(PlasterError):
    CODE: ClassVar[str] = "P012"

@dataclass(frozen=True)
class P013Error(PlasterError):
    CODE: ClassVar[str] = "P013"

@dataclass(frozen=True)
class P014Error(PlasterError):
    CODE: ClassVar[str] = "P014"

@dataclass(frozen=True)
class P015Error(PlasterError):
    CODE: ClassVar[str] = "P015"

@dataclass(frozen=True)
class P016Error(PlasterError):
    CODE: ClassVar[str] = "P016"

@dataclass(frozen=True)
class P020Error(PlasterError):
    CODE: ClassVar[str] = "P020"

@dataclass(frozen=True)
class P021Error(PlasterError):
    CODE: ClassVar[str] = "P021"

@dataclass(frozen=True)
class P030Error(PlasterError):
    CODE: ClassVar[str] = "P030"

@dataclass(frozen=True)
class P031Error(PlasterError):
    CODE: ClassVar[str] = "P031"
    source_file: Path | None = None
    source_annotations: tuple[SourceAnnotation, ...] = ()

@dataclass(frozen=True)
class P032Error(PlasterError):
    CODE: ClassVar[str] = "P032"
    source_file: Path | None = None
    source_annotations: tuple[SourceAnnotation, ...] = ()

@dataclass(frozen=True)
class P040Error(PlasterError):
    CODE: ClassVar[str] = "P040"

@dataclass(frozen=True)
class C001Error(PlasterError):
    CODE: ClassVar[str] = "C001"

# ── Per-code warning classes ────────────────────────────────────────────────

@dataclass(frozen=True)
class W001Warning(PlasterWarning):
    CODE: ClassVar[str] = "W001"

@dataclass(frozen=True)
class W002Warning(PlasterWarning):
    CODE: ClassVar[str] = "W002"

@dataclass(frozen=True)
class W003Warning(PlasterWarning):
    CODE: ClassVar[str] = "W003"

@dataclass(frozen=True)
class W004Warning(PlasterWarning):
    CODE: ClassVar[str] = "W004"
```

`toml_annotations` is a sequence of `TomlAnnotation` objects identifying each
annotated span in the substitution block; `render_diagnostic` iterates it to
break the `Syntax` block and insert annotation lines at the correct positions.
For P031 and P032, `source_annotations` holds one `SourceAnnotation` per match
found, encoding its position, span length, label, and whether it is excess.

#### Exception transport

Diagnostic objects travel from `PlasterFile` to `Compiler` inside exceptions.
Three exception classes form the transport hierarchy; each wraps exactly one
diagnostic:

```python
class PlasterDiagnosticException(Exception):
    """Base — carries a diagnostic for Compiler to handle."""

class PlasterFileException(PlasterDiagnosticException):
    """File-wide failure (P001, P002, P040). Compiler skips remaining substitutions."""
    def __init__(self, error: PlasterError): self.error = error

class PlasterSubstitutionException(PlasterDiagnosticException):
    """Substitution-level error. Compiler records it and calls resume()."""
    def __init__(self, error: PlasterError): self.error = error

class PlasterWarningException(PlasterDiagnosticException):
    """Warnings raised after a substitution completes. Compiler resumes."""
    def __init__(self, warnings: list[PlasterWarning]): self.warnings = warnings

class PlasterOutOfDateException(PlasterDiagnosticException):
    """Raised after all substitutions complete in dry_run mode when the result
    differs from disk. Carries a C001Error. Compiler does not resume."""
    def __init__(self, error: C001Error): self.error = error
```

`PlasterFileException` and `PlasterSubstitutionException` both carry a
`PlasterError`; `PlasterWarningException` carries a `list[PlasterWarning]`.
`PlasterOutOfDateException` carries a `C001Error` and is only raised
when `dry_run=True`. The distinction between the first two is solely about
scope — file vs. substitution — and determines how `Compiler` reacts.

#### PlasterFile.apply() and PlasterFile.resume()

The existing `PlasterFile` is a `@dataclass`. The new implementation requires
mutable instance state (`_current`, `_dry_run`, `_substitutions`,
`_source_contents`, `_patch_info`), so **remove the `@dataclass` decorator and
convert `PlasterFile` to a plain class with a regular `__init__`**.

`PlasterFile` processes substitutions one at a time and communicates every
diagnostic outcome through exceptions. It tracks the index of the currently
active substitution so that `resume()` can continue from the next one.

```python
class PlasterFile:
    def __init__(self, path: Path):
        """path: absolute path to the .toml plaster file."""
        self.path = path

    def _would_change(self) -> bool:
        """True if running substitutions would modify the source or patch file on disk."""
        ...

    def apply(self, dry_run: bool = False):
        """Start processing from substitution 0."""
        self._dry_run = dry_run
        self._current = 0
        self._process_from(0)

    def resume(self):
        """Advance past the current substitution and continue."""
        self._process_from(self._current + 1)

    def _process_from(self, start: int):
        for i in range(start, len(self._substitutions)):
            self._current = i
            self._apply_substitution(self._substitutions[i])
            # Warnings are checked after the substitution is fully applied,
            # so that a warning condition never interrupts a successful write.
            self._check_warnings(self._substitutions[i])
        # After all substitutions complete, check whether the result differs
        # from disk. Only raised in dry_run mode; never interrupts the loop.
        if self._dry_run and self._would_change():
            raise PlasterOutOfDateException(self._make_c001_diagnostic())
```

`_apply_substitution` raises `PlasterSubstitutionException` on error.
`_check_warnings` collects all warning conditions for the current substitution
and raises `PlasterWarningException` with the full list at once.
Both halt `_process_from` at the current index, leaving `_current`
pointing at the substitution that stopped. `resume()` increments past it.

**Source accumulation and cascading errors:** `_apply_substitution()` updates
`self._source_contents` in place — each successful substitution feeds its
output as input to the next. When a substitution raises
`PlasterSubstitutionException` (e.g. P030 — pattern not found), the source is
left unchanged for that step and subsequent substitutions operate on the text as
accumulated so far. Interdependent substitutions may therefore fail as a
cascade; this is expected and requires no special handling.

**`_would_change()` implementation:** This method consolidates what is currently
spread across `PatchInfo.save_source_if_changed(dry_run=True)` and
`PatchInfo.save_patch_if_changed(dry_run=True)`. Call both with `dry_run=True`
and return `True` if either would have written a file. Since `PatchInfo` already
owns the checksum logic, `_would_change()` is a thin wrapper. It belongs on
`PlasterFile` where it has access to the `PatchInfo` instance and the
accumulated `self._source_contents`.

**`find_all()` is unchanged.** The existing class method `PlasterFile.find_all()`
is retained as-is and is called by `get_plaster_files()` when no explicit file
paths are provided — it discovers all `.toml` files under `PLASTER_FILES_PATH`.
No modifications to `find_all()` are required.

`PlasterFileException` is raised by `apply()` before the substitution loop
begins (P001, P002, P040). It is never raised inside `_process_from`.

#### Compiler

`Compiler` drives the loop over all `PlasterFile` instances. It owns the
spinner, the diagnostic record, and the final summary.

The inner `_run` method uses a trampoline: `call` starts as `pf.apply` and
switches to `pf.resume` after each sub-level exception, so the same
exception-handling block covers the entire file regardless of how many
interruptions occur.

```python
class Compiler:
    def __init__(self, console: Console):
        self._console = console
        self._errors: list[PlasterError] = []
        self._warnings: list[PlasterWarning] = []
        self._files_with_errors: set[Path] = set()
        self._printed_headers: set[Path] = set()

    def build(self, plaster_files: list[PlasterFile],
              dry_run: bool = False) -> int:
        n = len(plaster_files)
        spinner = (self._console.status('')
                   if self._console.is_terminal else nullcontext())
        with spinner as status:
            for i, pf in enumerate(plaster_files, 1):
                label = pf.path.relative_to(PLASTER_FILES_PATH.parent)
                if self._console.is_terminal:
                    status.update(f'({i}/{n}) {label}')
                self._run(pf, dry_run=dry_run)
        self._render_summary(n)
        return 1 if (self._errors or self._warnings) else 0

    def _run(self, pf: PlasterFile, dry_run: bool = False):
        call = lambda: pf.apply(dry_run=dry_run)
        while True:
            try:
                call()
                return
            except PlasterOutOfDateException as e:
                self._record_error(e.error, pf)
                return                            # file processed but stale
            except PlasterFileException as e:
                self._record_error(e.error, pf)
                return                            # abort file
            except PlasterSubstitutionException as e:
                self._record_error(e.error, pf)
                call = pf.resume                  # continue from next sub
            except PlasterWarningException as e:
                for w in e.warnings:
                    self._record_warning(w, pf)
                call = pf.resume                  # continue from next sub

    def _print_file_header_if_needed(self, pf: PlasterFile):
        if pf.path not in self._printed_headers:
            self._printed_headers.add(pf.path)
            self._console.print(f'plaster apply {pf.path}', style='bold')

    def _record_error(self, error: PlasterError, pf: PlasterFile):
        self._errors.append(error)
        self._files_with_errors.add(pf.path)
        self._print_file_header_if_needed(pf)
        error.render(self._console)

    def _record_warning(self, warning: PlasterWarning, pf: PlasterFile):
        self._warnings.append(warning)
        self._print_file_header_if_needed(pf)
        warning.render(self._console)
```

#### Rendering

Each per-code diagnostic class owns its own rendering logic by overriding
`render(self, console: Console) -> None`. `Compiler` calls `d.render(console)`
without knowing the concrete type — standard polymorphism, no external dispatch
function. This makes rendering tests straightforward: construct a diagnostic
object, call `render`, and assert on the captured output.

The header line (`error[PXXX]: message`) is built from `self.CODE` and
`self.message`. Within `render`, `isinstance(self, PlasterWarning)` distinguishes
colour choices where needed (e.g., `bold on dark_goldenrod` vs `bold on dark_red`).
Each `render` reads the instance fields it needs (`toml_line_range`,
`toml_annotations`, `notes`, `hints`, and for P031/P032: `source_file`,
`source_annotations`) directly from `self`.

### TOML line positions

`tomllib` does not expose byte offsets or line numbers for individual keys.
Line positions are obtained by scanning the raw TOML text directly:

1. Split `plaster_contents` on `\n` to get all lines.
2. Find `[[substitution]]` markers to determine the byte range of each
   substitution block.
3. Within that range, locate specific key names (`description`, `pattern`,
   `re_pattern`, etc.) by line-by-line scan.

This approach is reliable for the simple, one-key-per-line TOML structure that
plaster files use. If a value spans multiple lines (triple-quoted TOML strings),
the annotation points to the first line of the value.

### Syntax block rendering

The substitution block is rendered as one or more `rich.syntax.Syntax` objects
sharing the same source string (`plaster_contents`) but with different
`line_range` arguments. Between segments, plain `rich.Text` annotation lines
are printed.

**Box-side prefix:** Every line emitted inside a diagnostic box — including all
`Syntax` blocks and all annotation `Text` lines — must be wrapped with a `| `
prefix in `bold blue`. The code examples below omit this prefix for readability.
In the implementation, use two helpers (reference implementations are in the
demo):

- `_prefixed(text: Text) -> Text` — prepends `"| "` in `bold blue` to a plain
  `Text` line.
- `_prefixed_syn(syn: Syntax, console: Console) -> Text` — renders the `Syntax`
  object to a temporary ANSI buffer, splits on newlines, and returns a single
  `Text` where each line starts with `"| "` in `bold blue`.

An example render sequence for P013:

```python
# Segment A: lines before the 'pattern' line
console.print(Syntax(content, "toml", line_numbers=True,
                     line_range=(sub_start, pattern_line - 1)))
# Annotation for 'pattern'
gutter = " " * gutter_width
val_indent = " " * len("pattern = '")
console.print(Text(f"{gutter}{val_indent}{'~'*len(pattern_value)} remove one",
                   style="bold red"))
# Segment B: 're_pattern' line only
console.print(Syntax(content, "toml", line_numbers=True,
                     line_range=(re_pattern_line, re_pattern_line)))
# Annotation for 're_pattern'
console.print(Text(f"{gutter}{re_val_indent}{'~'*len(re_pattern_value)} or this one",
                   style="bold red"))
# Segment C: remaining lines
console.print(Syntax(content, "toml", line_numbers=True,
                     line_range=(re_pattern_line + 1, sub_end)))
```

`gutter_width` is derived by rendering one line of the content through a
`force_terminal=True` Console, parsing the output with `Text.from_ansi()`, and
finding the `│` character in `text.plain`. `gutter_width = plain.index("│") + 1`.
This is the same code-path as `Text.stylize()` so the measured column is
guaranteed to match the actual character positions.

> **Note on alignment arithmetic:** the `+1` in `plain.index("│") + 1` means
> `gutter_width` points one character *past* `│`, not past the space that
> follows it. The downstream formulas (e.g. `gutter_width + error_col` for
> P001, `gutter_width + len("re_pattern = '") + re_error.pos` for P020) may
> look off-by-one on paper depending on whether the col value is 0-based or
> 1-based, but the demo renders correctly in practice. If you encounter a
> column misalignment during implementation, treat the demo's rendered output
> as the ground truth and adjust the formula to match it rather than deriving
> the offset from first principles.

### TOML parse position

`TOMLDecodeError` includes the error line and 1-based column in its message.
When rendering P001, the caret column within the annotation line is:

```
gutter_width + error_col
```

where `error_col` is the 1-based column extracted from the `TOMLDecodeError`
message with `r"at line (\d+), col(?:umn)? (\d+)"`.

### Regex position

`re.error` exposes `.pos` (0-based character index into the pattern string).
When rendering P020, the caret column within the annotation line is:

```
gutter_width + len("re_pattern = '") + re_error.pos
```

### Match position extraction for P031 and P032

Run `re.finditer()` on the source before the substitution to collect match
positions. Convert each `(match.start(), match.end())` byte offset pair to
`(1-based lineno, 0-based col_start, match_len)` using a helper that counts
`\n` characters up to each offset. Construct a `SourceAnnotation` for each
match (setting `is_excess=True` for matches beyond the expected count) and
store them in `source_annotations` on the `P031Error` or `P032Error` object.

### Source line display

Limit context to `CONTEXT_LINES_PER_MATCH = 3` lines above each match. Separate
match groups more than 5 lines apart with an ellipsis: `Text("    ...",
style="dim")` prefixed with `"| "` (bold blue via `_prefixed()`), rendering as
`|     ...`.

### Source match highlight

Applying a background colour to the matched substring while preserving the
surrounding Syntax token colours requires a round-trip through ANSI:

1. **Create a temporary `force_terminal=True` console.** The global console
   may be non-TTY (pipe, CI), which would suppress the `│` gutter character.
   The temporary console forces TTY mode so the gutter is always present and
   its width is always measurable.

   ```python
   tmp = Console(file=io.StringIO(), force_terminal=True,
                 force_jupyter=False, width=console.width,
                 color_system="truecolor")
   ```

2. **Render the single match line through Syntax.** Use the full source
   string with `line_range=(lineno, lineno)` so the line number gutter is
   rendered by Rich, not built by hand. Capture the ANSI output.

   ```python
   with tmp.capture() as cap:
       tmp.print(Syntax(source, lang, line_numbers=True,
                        line_range=(lineno, lineno)), end="")
   raw = cap.get()
   ```

3. **Parse the ANSI back into a `rich.Text`.** This gives a Text object whose
   style spans encode the Syntax token colours.

   ```python
   colored = Text.from_ansi(raw.rstrip("\n"))
   ```

4. **Measure `gutter_width` from `colored.plain`.** Use the `.plain` property
   of the `Text` object produced in step 3 — this is the same ANSI-parsing
   code-path as `Text.stylize()`, so the measured `│` position is guaranteed to
   match the character positions used by `stylize()`:

   ```python
   plain = colored.plain
   if "│" in plain:
       gutter_width = plain.index("│") + 1
   else:
       gutter_width = len(str(len(source.splitlines()))) + 3  # fallback
   ```

5. **Overlay the match background with `Text.stylize()`.** The `stylize()`
   call adds a new style span on top of the existing token spans. When the
   new style includes a background colour, it overrides the Syntax theme
   background for exactly those characters while the rest of the line keeps
   its original token colour and background.

   ```python
   bg = "bold on dark_green" if expected else "bold on dark_red"
   colored.stylize(bg,
                   gutter_width + match_start,
                   gutter_width + match_start + match_len)
   ```

6. **Print the result directly** — the gutter is already included from the
   Syntax render, so no manual `num_str + " │ "` construction is needed.

### Exit codes

| Situation               | Exit code |
|-------------------------|-----------|
| All files OK            | 0         |
| Any error or warning    | 1         |
| Unhandled exception     | 1 (Python default — traceback to stderr) |

---

## Testing

### Test files

Test files follow the same conventions as the rest of the `tools/cr/` suite:

- **Named `*_test.py`** — the suffix is what `vpython3` and the test runner use
  to discover test files.
- **`vpython3` shebang** — the first line must be `#!/usr/bin/env vpython3` so
  the file is runnable directly.
- **Executable bit** — run `git update-index --chmod=+x <file>` after creating
  the file so it is marked executable in the repository.
- **Runnable standalone** — `vpython3 tools/cr/plaster_error_test.py` must
  execute the full test suite without any additional arguments or wrapper script.

All error-machinery tests live in `plaster_error_test.py`, separate from the
existing `plaster_test.py`. Two test classes cover distinct concerns:

| Class                        | What it tests                                              |
|------------------------------|------------------------------------------------------------|
| `PlasterErrorRenderingTest`  | `render_diagnostic()` output structure — no fake repo needed |
| `PlasterErrorIntegrationTest`| Each error/warning code triggered via a real fake repo     |

### Rendering tests (`PlasterErrorRenderingTest`)

Construct diagnostic objects directly with known field values and call
`d.render(console)`. Use a non-TTY console so the output is plain text with no
ANSI codes, making assertions straightforward:

```python
buf = io.StringIO()
console = Console(file=buf, force_terminal=False, highlight=False)
error.render(console)
out = buf.getvalue()
self.assertIn('error[P030]', out)
self.assertIn('namespace net', out)
self.assertIn('= note:', out)
```

One test per error code is the minimum. Tests for the annotation alignment
helpers (`_ann`, `_toml_match_line`, `_src_match_line`) may be unit-tested
separately with known content strings.

### Integration tests (`PlasterErrorIntegrationTest`)

Use `FakeChromiumSrc` exactly as `PlasterTest` does — `setUp` calls
`self.fake_chromium_src.setup()` and registers `cleanup` via `addCleanup`.
Plaster runs against real files in the temp directories; no filesystem
operations are mocked.

**Do not** use `patch('sys.stderr')` or `patch('sys.exit')`. Diagnostics are
collected by `Compiler` and returned as lists; tests assert on those lists
directly.

A private helper keeps test bodies short:

```python
def _apply(self, toml: str, source: str = '',
           path: str = 'net/test.cc',
           dry_run: bool = False) -> list[plaster.PlasterDiagnostic]:
    src = Path(path)
    if source:
        self.fake_chromium_src.write_and_stage_file(
            src, source, self.fake_chromium_src.chromium)
        self.fake_chromium_src.commit(f'add {src.name}',
                                      self.fake_chromium_src.chromium)
    plaster_path = plaster.PLASTER_FILES_PATH / (str(src) + '.toml')
    plaster_path.parent.mkdir(parents=True, exist_ok=True)
    plaster_path.write_text(toml)
    buf = io.StringIO()
    console = Console(file=buf, force_terminal=False)
    compiler = plaster.Compiler(console)
    compiler.build([plaster.PlasterFile(plaster_path)], dry_run=dry_run)
    return compiler._errors + compiler._warnings
```

A typical test then reads:

```python
def test_P030_no_matches(self):
    diags = self._apply(
        toml="[[substitution]]\ndescription='d'\npattern='MISSING'\n"
             "replace='x'\ncount=1\n",
        source="no match here")
    self.assertEqual(len(diags), 1)
    self.assertIsInstance(diags[0], plaster.P030Error)
    self.assertEqual(diags[0].substitution_index, 0)
```

For P040 (`source-not-found`), call `_apply` with no `source` argument so the
source file is never created. For P001 (`toml-parse-error`), write malformed
TOML and omit the `source` argument.

### Coverage matrix

Every error and warning code must have at least one integration test and one
rendering test:

| Code | Trigger condition to set up                                        |
|------|--------------------------------------------------------------------|
| P001 | Malformed TOML (unquoted value, missing bracket, etc.)             |
| P002 | Valid TOML with no `[[substitution]]` tables                       |
| P010 | Substitution with no `description` key                             |
| P011 | Substitution with no `pattern` or `re_pattern` key                 |
| P012 | Substitution with no `replace` key                                 |
| P015 | Substitution with no `count` key                                   |
| P016 | `count` set to a negative integer or a non-integer value           |
| P013 | Substitution with both `pattern` and `re_pattern`                  |
| P014 | `re_flags` containing an unknown flag name                         |
| P020 | `re_pattern` that fails `re.compile()`                             |
| P021 | `re_pattern` with no capture groups; `replace` uses `\1` back-reference |
| P030 | Pattern present in source zero times (with any `count` value)      |
| P031 | Pattern matches M times, `count` is N, M > N                       |
| P032 | Pattern matches M times, `count` is N, 0 < M < N                  |
| P040 | TOML references a source path that does not exist on disk          |
| C001 | Run `_apply(..., dry_run=True)` after `apply()` succeeds; source or patch would differ |
| W001 | `description` value longer than 80 characters                      |
| W002 | Multi-line `description` with no blank line after the summary      |
| W003 | `count = 0` with no `# reason:` comment on the same line          |
| W004 | `pattern` alongside `re_flags`                                     |

Additionally, one test must verify **resilient collection**: a TOML with two
substitutions where both trigger independent errors (e.g., P011 followed by
P012) must produce `len(errors) == 2` with the correct codes in order, and
neither error must prevent the other from being reported.

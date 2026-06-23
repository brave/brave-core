---
name: plaster-from-patch
description:
  "Convert an existing Brave-core Chromium `.patch` into a Plaster
  (`rewrite/*.yaml`) config that reproduces the same change, following the
  Plaster Dos and Don'ts. Triages each hunk (plaster vs chromium_src), authors
  intent-conveying substitutions, then verifies with `plaster.py`. Triggers on:
  plaster from patch, convert patch to plaster, plasterize, make a plaster."
argument-hint:
  '<patch file, source path, or nothing to infer from working tree>'
disable-model-invocation: true
allowed-tools:
  Bash(python3:*), Bash(vpython3:*), Bash(git:*), Bash(npm:*), Bash(ls:*),
  Bash(cat:*), Bash(grep:*), Bash(diff:*), Bash(find:*), Read, Grep, Glob, Edit,
  Write, Agent
---

# Plaster From Patch

Turn a regular Brave-core Chromium patch into a 🩹 Plaster config. The user has
already produced a normal patch the usual way — edited an upstream source under
`src/`, then ran `npm run update_patches` to generate
`patches/<dashed-path>.patch`. This skill reads that change and writes the
equivalent `rewrite/<path>.yaml`, following the rules below, so the same effect
is achieved through a robust regex substitution instead of a brittle context
patch.

## Required reading

The conversion rules come from two docs. Read them if present — they are the
source of truth and may be updated:

- `docs/plaster.md` — how Plaster works, file layout, the substitution schema.
- `docs/plaster_dos_and_donts.md` — the dos and don'ts (introduced in
  brave/brave-core#36525). If this file is not yet on your branch, the
  **Conversion rules** section below embeds its essentials so the skill still
  works.

## Mental model

| Artifact        | Location                                      | Role                                         |
| --------------- | --------------------------------------------- | -------------------------------------------- |
| Upstream source | `../<path>` (e.g. `../components/foo/foo.cc`) | The pristine Chromium file, from `git`       |
| Patch           | `./patches/<path-with-/-as->.patch`           | Encodes upstream → modified                  |
| Plaster         | `./rewrite/<path>.yaml`                       | Regex substitutions you author               |
| `chromium_src`  | `./chromium_src/<path>`                       | Shadow file for file-scope / substantive C++ |

Key facts that drive everything:

- **Plaster loads the source from `git`, not disk.** Your regex must match the
  **pristine upstream** text (the patch's `a/` side) and transform it into the
  **modified** text (the `b/` side). Running `plaster.py apply` regenerates the
  `.patch` from the plaster.
- The patch stem replaces `/` with `-`: `components/foo/foo.cc` →
  `patches/components-foo-foo.cc.patch`. The plaster keeps the real path:
  `rewrite/components/foo/foo.cc.yaml`.

## Step 1 — Resolve the input and gather the change

This skill runs from `src/brave`. Confirm with `git rev-parse --show-toplevel`.

Resolve the argument into one or more `(source_path, patch_path)` pairs:

- **A `.patch` file** (e.g. `patches/components-foo-foo.cc.patch`): read it; the
  `diff --git a/<src> b/<src>` header gives the source path.
- **A source path** (e.g. `components/foo/foo.cc`): the patch is
  `patches/<path with / → ->.patch`. If there is no patch yet but the working
  tree under `../` has uncommitted edits to that source, treat the working-tree
  diff (`git -C .. diff -- <path>`) as the change.
- **No argument**: infer. List candidate patches via `git status --short` in
  this repo (new/modified files under `patches/`) and modified sources via
  `git -C .. status --short`. If there is exactly one clear candidate, use it;
  if several, list them and ask which to convert.

For each pair, obtain three things:

1. **The unified diff** — `cat patches/<...>.patch`, or
   `git -C .. diff -- <path>`.
2. **The pristine upstream text** of every region the diff touches — the patch's
   `a/` side, or `git -C .. show HEAD:<path>`. This is what your regex matches
   against. Read enough surrounding context (the whole enclosing function, enum,
   switch, or gn target) to anchor on intent, not coincidence.
3. **The intended modified text** — the `b/` side, or the working-tree file.

If `plaster.py` cannot resolve the source from `git` (uncommitted, or the file
is not in Chromium's `src` repo — only `src` is supported), say so and stop:
Plaster cannot patch it.

## Step 2 — Triage each hunk: plaster, `chromium_src`, or both

**Not every patch belongs in a plaster.** Before writing any regex, classify
each hunk. This is the single most important step — getting it wrong produces a
plaster the reviewers will reject.

- **Mere file-scope addition** — a new `#include`, an anonymous-namespace
  constant or helper, a new type, anything that can live _before or after_ the
  upstream `#include` of the file. → **Do NOT plaster.** Put it in a
  `chromium_src/<path>` shadow file that re-includes the upstream source.
  Plaster is only for changes that cannot be made at file scope.

- **Substantive C++ injected into a function/class** — more than a trivial
  one-liner. → Put the substantive code in a `chromium_src` shadow helper
  (anonymous-namespace function, etc.) so it gets `clang-format`, presubmit,
  DEPS, and semgrep coverage; keep the plaster a **minimal call-site insertion**
  (`MaybeApplyBar(this);`).

- **Trivial in-place change** — flipping a value, redirecting a constant,
  inserting one short line, extending an enum/switch, extending a gn array,
  renaming one definition. → **Author a plaster substitution** (Step 3). Be
  pragmatic: if the whole change is one clear line, let the plaster carry it
  directly rather than splitting it across a `chromium_src` file for no gain.

Produce an explicit triage list: for each hunk, which bucket and why. Show it to
the user before writing files — this is where the human judgment lives.

## Step 3 — Author the plaster substitutions

Write `rewrite/<path>.yaml`. Start from the standard MPL header (copy it from
any existing `rewrite/**/*.yaml`), then a `substitutions:` list. Apply these
rules — they come straight from the dos and don'ts.

### Conversion rules

**Convey intent, match the minimal uniquely-identifying context.** The regex is
how the change's intent is recorded. Anchor on the things that describe _what_
is changing — the function name, the switch variable, the gn target, the
position (top / before `kMaxValue` / before `default:`) — and nothing more.
Every extra incidental particular is a future false break; every missing one is
an accidental match elsewhere.

**`pattern` vs `re_pattern`.** Use `pattern` (a literal string, auto-escaped)
for simple global **symbol** replacement (`kOldConst` → `kNewConst`,
`ChromiumMethod` → `BraveMethod`). Use `re_pattern` the moment you need context,
capture groups, or whitespace flexibility.

**Whitespace.** Don't depend on specific spacing — prefer `\s*` / `\s+` over
literal runs, and pair flexible whitespace with `re_flags: [DOTALL]` so `.`
crosses newlines (and the regex reads better). A single literal space is
acceptable only when it materially improves readability, the file is
auto-formatted, and that space can never become a newline. Use single-quoted
YAML for patterns so `\s` stays two characters.

**Enums.** Anchor at the top or just before `kMaxValue`; do **not** anchor on a
neighbor key (it may be renamed/removed). Take care of `kMaxValue` and trailing
commas. If the enum value is serialized/persisted, key positions are not free —
handle case by case.

```yaml
- description: 'Append Brave entries before kMaxValue'
  re_pattern: '(enum class RequestType \{.+?,)(\s+)kMaxValue = \w+'
  re_flags: [DOTALL]
  replace: |-
    \1
      kBraveOne,
      kBraveTwo,
      kMaxValue = kBraveTwo
```

**Switch statements.** Match the **function**, then the **switch block**, to
place the insertion — never anchor on a sibling `case` key. If a `default:`
exists (or could), treat it as the end boundary and insert before it. To group a
new case with fall-through siblings, anchor on the shared outcome (the
`return`), not on a case key.

```yaml
- description: 'Add ECDSA_SHA384 handling to VerifyInit'
  re_pattern: '(VerifyInit\([^)]*\)\s*\{.*?\n\s*switch[^{]*\{.*?)(\n\s*\})'
  re_flags: [DOTALL]
  replace: |-
    \1
        case ECDSA_SHA384:
          digest = EVP_sha384();
          break;\2
```

**gn arrays.** Extend `sources` / `deps` / `public_deps` with `+=` **immediately
after instantiation** (they get passed onward, so extend before any use). One
substitution per array — don't bundle. Scope the match to the specific target.

```yaml
- description: 'Append Brave sources to component("foo")'
  re_pattern: '(component\("foo"\).*?sources\s*=\s*\[.*?\])'
  re_flags: [DOTALL]
  replace: '\1\n  sources += brave_foo_sources'
```

**`count`.** Defaults to `1` and shouldn't be spelled out. Intentional breakage
is a feature: when the count stops matching, the change gets re-reviewed. Use
`count: 0` only when occurrences legitimately vary over time **and** every match
is inherently safe to substitute — and add a comment saying why. Never raise
`count` just to silence a "matched N times" failure; investigate it.

**Prefer plaster over the alternatives, and remember why** — so the description
can carry it: a `.patch` breaks when adjacent lines move; a `#define` silently
rewrites every occurrence including ones pulled in by `#include`. A scoped
plaster substitution rewrites exactly the one place its match describes.

### Descriptions

Give each substitution a `description` that states the intent — what is changing
and why — not a restatement of the regex mechanics. Use a `|` / `|-` block
scalar for multi-line text. This is the recovery note for whoever hits a future
break.

## Step 4 — Write any `chromium_src` shadow file

For hunks triaged to `chromium_src` in Step 2, create/extend
`chromium_src/<path>`: file-scope additions and helpers go before, then
`#include <path>` re-includes the upstream source (angle-bracket include of the
original). Match the surrounding conventions of existing `chromium_src` files
(MPL header, include guards where relevant). Keep the matching plaster down to
the minimal call-site insertion.

## Step 5 — Apply and verify (the acid test)

The plaster is correct **iff** applying it to the pristine upstream source
reproduces the user's intended modified source exactly. Verify, don't assume.

`apply` / `check` take the **plaster or patch path**, never the chromium source
path — passing `../<source_path>` raises `ValueError: Unexpected file path`.
Accepted forms are `rewrite/<path>.yaml` and `patches/<dashed>.patch` (or no
argument to process every plaster):

```bash
tools/cr/plaster.py check rewrite/<path>.yaml   # dry run
tools/cr/plaster.py apply rewrite/<path>.yaml   # rewrite ../<source>, regen patch + .patchinfo
```

`apply` writes the transformed text back to the chromium source on disk at
`../<source_path>` and regenerates `patches/<dashed>.patch` + `.patchinfo`.

- `plaster.py` needs PyYAML. If it fails with
  `ModuleNotFoundError: No module named 'yaml'`, retry with the depot_tools
  interpreter (`vpython3 tools/cr/plaster.py ...`) or install into the active
  env (`python3 -m pip install pyyaml`). If it still cannot run here, **do not
  claim success** — manually simulate each substitution against the pristine
  text (Python `re` reasoning), state that local verification was unavailable,
  and flag that CI / the user must run `plaster.py check`.

**Compare results.** Stash the user's intended modified source first (so
`apply`, which rewrites the source from `git` + the plaster, doesn't clobber
it), then compare what the plaster produced against it:

```bash
cp ../<source_path> /tmp/intended.txt        # the user's hand-edited target
tools/cr/plaster.py apply rewrite/<path>.yaml # rewrites ../<source_path> from git+plaster
diff /tmp/intended.txt ../<source_path>       # must be empty
```

The plaster-produced source must be byte-identical to the intended modified
source (empty `diff`). If it differs, the regex matched the wrong region or the
replacement is off — fix the substitution and re-run. Iterate until the match is
exact and `count` matched as expected (the default `1`).

Once it matches, the original hand-written `patches/<dashed>.patch` is now
**generated by the plaster** — it stays (plaster maintains it), but it is no
longer hand-maintained. Make sure `.patchinfo` was written alongside it.

## Step 6 — Self-review

Before declaring done, run the `/code-review` skill (or an Agent review) over
the new/changed files (`rewrite/<path>.yaml`, any `chromium_src/<path>`, the
regenerated patch). Specifically check the conversion-rule violations that are
easy to miss:

- Anchoring on a removable neighbor key (enum/switch) instead of structure.
- A `re_pattern` that depends on incidental whitespace or unrelated lines.
- A `count: 0` without a justifying comment, or a silenced count mismatch.
- File-scope additions that should have been `chromium_src`, not plaster.
- Substantive C++ inlined in `replace` that belongs in a shadow helper.
- Bundled gn array extensions, or extensions not placed right after declaration.

Fix anything it surfaces before moving on.

## Step 7 — Commit, branch, PR

Stage the plaster, any `chromium_src` file, and the regenerated
patch/`.patchinfo`. Commit on the working branch (new commit; don't squash onto
unrelated work):

```bash
git add rewrite/<path>.yaml chromium_src/<path> patches/<dashed>.patch patches/<dashed>.patchinfo
git commit -m "Convert <path> patch to plaster"
```

End the commit message with the standard trailer:
`Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>`.

Then offer to push and open a PR (`gh pr create`). Report the URL.

## Guidelines

- **Triage first, author second.** Most rejected plasters are changes that never
  belonged in a plaster. Decide plaster vs `chromium_src` before touching regex.
- **Match the pristine upstream, not the patched file** — Plaster reads the
  source from `git`.
- **Verify by reproduction.** A plaster that "looks right" but doesn't reproduce
  the exact modified source is wrong. Run `plaster.py` and compare.
- **Let intentional breakage stand.** Don't paper over a `count` mismatch; it is
  the safety mechanism working.
- **Keep the regex minimal and intent-bearing.** The smallest match that is
  still unique is the most robust across rebases.
- **Format docs/markdown with `npm run format`** if you touch any.

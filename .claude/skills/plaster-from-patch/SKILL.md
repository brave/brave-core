---
name: plaster-from-patch
description:
  "Create a Plaster (`rewrite/*.yaml`) config for an existing Brave-core
  Chromium `.patch`, authoring an intent-conveying regex match per the Plaster
  Dos and Don'ts, then verifying it by running `plaster.py apply` and diffing
  the regenerated patch against the original. Does not modify sources or create
  other files. Triggers on: plaster from patch, convert patch to plaster,
  plasterize, make a plaster."
argument-hint:
  '<patch file, source path, or nothing to infer from working tree>'
disable-model-invocation: true
allowed-tools:
  Bash(python3:*), Bash(vpython3:*), Bash(git:*), Bash(npm:*), Bash(ls:*),
  Bash(cat:*), Bash(grep:*), Bash(diff:*), Bash(find:*), Read, Grep, Glob, Edit,
  Write, Agent
---

# Plaster From Patch

Create a 🩹 Plaster config for a patch the user has already written. The user
made a normal Brave-core change the usual way — edited an upstream source under
`src/`, then ran `npm run update_patches` to generate
`patches/<dashed-path>.patch` — and now wants the equivalent
`rewrite/<path>.yaml` so the change is expressed as a robust regex substitution
instead of a brittle context patch.

## Scope — read this first

**The only thing this skill produces is the plaster config**
(`rewrite/<path>.yaml`).

- It does **not** modify the Chromium source, the patch, or any other file.
- It does **not** create `chromium_src` shadow files or move code around.
- It does **not** decide whether the change _should_ be a plaster — the user
  already decided that by writing the patch. Author a config that reproduces the
  patch, following the guidelines.

The one thing it does beyond writing the config is **verify** (Step 3): it runs
`plaster.py apply` to regenerate the patch from the config and confirms it is
identical to the patch the user wrote. That regenerates the patch and its
`.patchinfo` (plaster's own output) — it still never hand-edits sources or
creates `chromium_src` files.

## Required reading

The matching rules come from two docs. Read them if present — they are the
source of truth and may be updated:

- `docs/plaster.md` — how Plaster works, file layout, the substitution schema.
- `docs/plaster_dos_and_donts.md` — the dos and don'ts (introduced in
  brave/brave-core#36525). If this file is not yet on your branch, the
  **Matching rules** section below embeds its essentials so the skill still
  works.

## Mental model

| Artifact        | Location                                      | Role                                   |
| --------------- | --------------------------------------------- | -------------------------------------- |
| Upstream source | `../<path>` (e.g. `../components/foo/foo.cc`) | The pristine Chromium file, from `git` |
| Patch           | `./patches/<path-with-/-as->.patch`           | Encodes upstream → modified            |
| Plaster         | `./rewrite/<path>.yaml`                       | The regex config you author            |

Key facts that drive the match:

- **Plaster loads the source from `git`, not disk.** Your regex must match the
  **pristine upstream** text (the patch's `a/` side) and transform it into the
  **modified** text (the `b/` side).
- The patch stem replaces `/` with `-`: `components/foo/foo.cc` →
  `patches/components-foo-foo.cc.patch`. The plaster keeps the real path:
  `rewrite/components/foo/foo.cc.yaml`.

## Step 1 — Resolve the input and read the change

This skill runs from `src/brave`. Confirm with `git rev-parse --show-toplevel`.
This step only reads — it changes nothing.

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

If the source is not in Chromium's `src` repo (only `src` is supported), say so
and stop: Plaster cannot patch it.

If a hunk is a pure file-scope addition (a new `#include`, an
anonymous-namespace helper) that the dos and don'ts say is better hosted in a
`chromium_src` shadow file than in a plaster, you may **mention** that to the
user as a heads-up — but still author the config you were asked for and create
no other files. The choice is the user's.

## Step 2 — Author the plaster config

Write `rewrite/<path>.yaml` and nothing else. Start from the standard MPL header
(copy it from any existing `rewrite/**/*.yaml`), then a `substitutions:` list.
One substitution per logical change in the patch. Apply these rules — they come
straight from the dos and don'ts.

### Matching rules

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

### Descriptions

Give each substitution a `description` that states the intent — what is changing
and why — not a restatement of the regex mechanics. Use a `|` / `|-` block
scalar for multi-line text. This is the recovery note for whoever hits a future
break.

## Step 3 — Verify by regenerating the patch and diffing against the original

The config is correct **iff** running plaster produces a patch byte-identical to
the one the user wrote. Don't settle for a dry-run `check` — actually regenerate
the patch and compare it to the original:

```bash
# 1. Keep the user's original patch as the ground truth.
cp patches/<dashed>.patch /tmp/plaster-original.patch

# 2. Regenerate the patch from the config: this rewrites the source from
#    git + the plaster and writes patches/<dashed>.patch + .patchinfo.
tools/cr/plaster.py apply rewrite/<path>.yaml

# 3. The regenerated patch MUST equal the original — empty diff.
diff /tmp/plaster-original.patch patches/<dashed>.patch
```

An empty `diff` means the plaster reproduces the user's change exactly — done. A
non-empty diff (or a `count`/match error raised by `apply`) means the regex
matched the wrong region, the replacement is off, or `count` didn't match (found
0 or several). Fix the substitution and re-run the apply+diff. Iterate until the
diff is empty.

`apply` / `check` take the **plaster or patch path**, never the chromium source
path — passing `../<source_path>` raises `ValueError: Unexpected file path`.
Accepted forms are `rewrite/<path>.yaml` and `patches/<dashed>.patch`.

`plaster.py` needs PyYAML. If it fails with
`ModuleNotFoundError: No module named 'yaml'`, retry with the depot_tools
interpreter (`vpython3 tools/cr/plaster.py ...`) or install into the active env
(`python3 -m pip install pyyaml`). If it still cannot run here, **do not claim
success** — say local verification was unavailable and that the user must run
the apply+diff above.

## Step 4 — Self-review

Before declaring done, run the `/code-review` skill (or an Agent review) over
the new `rewrite/<path>.yaml`. Specifically check the matching-rule violations
that are easy to miss:

- Anchoring on a removable neighbor key (enum/switch) instead of structure.
- A `re_pattern` that depends on incidental whitespace or unrelated lines.
- A `count: 0` without a justifying comment, or a silenced count mismatch.
- A `replace` that diverges from the patch's intended text (re-diff against the
  patch).
- An overly broad `pattern` that would match in more places than intended.

Fix anything it surfaces before moving on.

## Step 5 — Commit and offer a PR

Stage the config plus the patch artifacts that `apply` regenerated in Step 3,
and commit on the working branch (new commit; don't squash onto unrelated work):

```bash
git add rewrite/<path>.yaml patches/<dashed>.patch patches/<dashed>.patchinfo
git commit -m "Add plaster config for <path>"
```

End the commit message with the standard trailer:
`Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>`.

The original hand-written patch is now plaster-managed (regenerated from the
config and tied to it by `.patchinfo`). Then offer to push and open a PR
(`gh pr create`). Report the URL.

## Guidelines

- **Config only.** The deliverable is `rewrite/<path>.yaml`. Don't edit sources,
  don't create `chromium_src` files, don't restructure the change.
- **Match the pristine upstream, not the patched file** — Plaster reads the
  source from `git`.
- **Verify by regeneration** — run `plaster.py apply` and diff the regenerated
  patch against the original; they must be identical. A config that "looks
  right" but doesn't reproduce the patch is wrong.
- **Let intentional breakage stand.** Don't paper over a `count` mismatch; it is
  the safety mechanism working.
- **Keep the regex minimal and intent-bearing.** The smallest match that is
  still unique is the most robust across rebases.
- **Format docs/markdown with `npm run format`** if you touch any.

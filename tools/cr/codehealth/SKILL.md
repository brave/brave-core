# Upstream code health review

Instructions for producing or updating a `tools/cr/codehealth/M<milestone>.md`
tracker from the upstream Chromium history that a `cr<milestone>` branch lifts
Brave across. The tracker lists what in the range Brave should fix, migrate to,
or imitate, as checkboxes to track progress.

Build breaks get fixed during the rebase. This review is for everything else,
and most of it fails silently: the tree compiles and tests pass, but a Brave
override has stopped applying, a Brave-disabled feature is back, or a Brave copy
of upstream code has missed a fix.

## Inputs

- **Range:** the Chromium tags the branch lifts across. The old tag is
  `projects.chrome.tag` in `package.json` on the branch's base: `origin/master`,
  or `origin/cr<N-1>` while the previous milestone hasn't landed (M159:
  `158.0.8127.2`). The new one is the same field on the branch, or the subject
  of its "Update patches from Chromium X to Y" commit. Review the whole range,
  not just its tail. Expect 4000 to 9000 commits. Run `git log` in `src/`, not
  in `src/brave`.
- **V8 range:** `v8_revision` in `src/DEPS` at both tags. Run that `git log` in
  `src/v8`.
- **Milestone:** the major version of the new tag. The tracker is
  `tools/cr/codehealth/M<milestone>.md`. Anything under `tools/cr/codehealth/`,
  this file, the trackers and the tools alike, is committed as
  `git commit -m 'Version CodeHealth tracker'`.
- **What the rebase already fixed:**
  `git log --format='%h %s%n%b' origin/master..origin/cr<milestone>` in
  `src/brave` (for a branch not on origin yet, `<base>..cr<milestone>` locally).
  These are the build fixes made during the rebase. Each one names the upstream
  commit it follows, and shows which patterns recur. A commit marked `[WIP]`
  isn't a fix yet: list what it leaves to verify as an item. Neither is a commit
  that drops a hook or a pin. After "Drop the X patch/override", grep that the
  Brave function the hook called still has a caller. After "Drop the X pin",
  read the commit that added the pin (`git log origin/master -S'<feature>'`) and
  check that upstream's clean-up didn't keep the branch Brave avoided.

If the tracker already exists, update it (see
[Updating a tracker](#updating-a-tracker)) rather than writing a new one.

## Ground rules

- Verify every claim statically against the tree: read the upstream diff, the
  Brave code and the headers. Don't build or run tests, and don't list an item
  without having checked that it touches Brave. Verify subagents' findings too
  before writing them in.
- `src/` has Brave's patches applied. Read pristine upstream with
  `git show <tag>:<path>`.
- An item belongs in the tracker only if Brave has code it affects: a call site,
  an override, a plaster, a patch, a `chromium_src` file, a lit_mangler, a
  Polymer modification, a test filter, or presubmit config. Upstream-only churn
  is not an item.
- Something the rebase already fixed is not an item. If the rebase only plumbed
  a new parameter through, check whether the behaviour change behind it still
  needs work.
- Check reverts. For every candidate, search the range for `Revert "<subject>"`
  and for a later reland. Only what's left standing counts.
- Don't trust a green Linux build: grep Brave's platform blocks for constants
  that "Restrict visibility" commits move. Brave sources listed only under
  `is_win`, `is_mac`, `is_android` or `is_ios` in `BUILD.gn` aren't compiled
  there.
- Name the upstream commit that motivates each item, linked as
  `[<hash>](https://chromium.googlesource.com/chromium/src/+/<hash>)`. V8
  changes live in `src/v8` and link to `.../v8/v8/+/<hash>`.
- Don't cite `cr<milestone>` branch hashes in the tracker. The branch is rebased
  daily, so they go stale within a day. Name a branch commit by its subject
  (e.g. "[cr157] Keep the system proxy resolver away from Tor profiles").
  Upstream Chromium and V8 hashes are stable, and so are Brave hashes on
  `origin/master`.

## Procedure

### 1. Collect the data

Dump the range once, and keep the files for every sweep (and for subagents). Use
a new directory per range: reusing one mixes a stale `claimed` file into step 3,
and leaves candidate lists of rules removed since.

```sh
A=<old tag>; B=<new tag>; D=/tmp/codehealth_$B
vpython3 tools/cr/codehealth/scripts/sweep.py $A..$B --out $D
```

It writes `subjects`, `files`, `bodies`, `range.diff` and `changes` to `$D`,
and, for each rule under `rules/<sweep>/`, the commits it applies to in
`$D/rules/<sweep>/<rule>.candidates` (see [Rules](#rules)). Grep `changes` for
names rather than running `git grep` at both tags, which times out when repeated
per name, or `git log -S`: each of its lines is `<hash> <path>:<+/- line>`, so a
hit names its commit. `range.diff` is the net diff, for what the range left.

The overlap list, the range's commits grouped by the upstream file Brave
overrides, is sweep [a](#a-overlap-commits-touching-what-brave-overrides)'s
`overridden-files` candidates (`tools/cr/codehealth/scripts/overlap.py`).

Brave's crbug and CL citations, matched against the range's `Bug:`, `Fixed:` and
`Reviewed-on:` lines, reverts and relands included, are rule
[obsolete/upstream-bugs-now-fixed](rules/obsolete/upstream-bugs-now-fixed.md)'s
candidates (`obsolete.py cited-bugs`).

### 2. Run the sweeps

Each sweep targets one way Brave gets exposed, and has its rules in
`rules/<sweep>/`. They're independent, so give each sweep to a subagent, with
the ground rules, its rule files and their `.candidates`. An incremental range
of a few hundred commits has short lists: sweeps can share a subagent, and a
range that is only the `chrome/VERSION` bump needs none. For each rule, it reads
the file, then checks the candidates as "How to check" says. Rules sharing a
list say so ("Same as …"); read it once. A list that says "read the whole range"
belongs to a rule with no patterns.

Ask each subagent for method notes (which rules found verified items, which
lists were noise, and any tooling problem it hit), and for the hashes of the
commits it read, one per line, to append to `$D/claimed` for step 3. Forks may
not be allowed to write report files, so ask for the items and notes in the
final reply as well. Don't start step 3 or write the tracker until every
subagent has sent its final reply: a notice that it still has background work
running is interim. Tell each subagent not to leave commands running in the
background once it has replied.

#### a. Overlap: commits touching what Brave overrides

The rules are in [`rules/overlap/`](rules/overlap/), with their candidates in
`$D/rules/overlap/`. The overlap list is grouped by top-level directory: split
it across subagents by directory. It leaves out build files, flag metadata,
histograms and strings; commits that only touch feature lists are better read in
sweep [b](#b-brave-disablements-and-defaults). It can't see Brave's Polymer
overrides in `browser/resources/**/br/`, which are keyed by element tag (sweep
[d](#d-webui)), nor test filters and `build/chromium/resources/` (sweep
[g](#g-obsolete-brave-code)).

#### b. Brave disablements and defaults

Brave turns upstream things off in many places, and upstream keeps moving them.
The rules are in [`rules/defaults/`](rules/defaults/), with their candidates in
`$D/rules/defaults/`. Android Java cached flags are in sweep
[h](#h-android-and-ios).

New upstream features that phone home or add Google-service UI aren't
code-health items. List them at the end of the tracker for the privacy team, one
line each.

#### c. Security hardening and forked code

The rules are in [`rules/security/`](rules/security/), with their candidates in
`$D/rules/security/`. Brave's forks are indexed by
`tools/cr/codehealth/scripts/forks.py`; add a fork found by hand to its
`ForkIndex.KNOWN`. Its last section lists copied functions that no longer match
their original at the new tag, when the range changed the original's function;
`--debt` lists those whose original it didn't change, for a fresh tracker.
Raw-pointer and other campaign enforcement belongs to sweep
[e](#e-enforcement-and-deprecations), not here.

#### d. WebUI

The rules are in [`rules/webui/`](rules/webui/), with their candidates in
`$D/rules/webui/`. `polymer_tags.py` indexes Brave's `polymer_overriding` tags,
which the override index can't see.

- **Data sources.** See the WebUI CORS item in sweep
  [c](#c-security-hardening-and-forked-code).
- Removed `loadTimeData` keys and handler names, lint rules and `$` typing all
  surface as build errors or crashes during the rebase. They need little
  attention here.

#### e. Enforcement and deprecations

The rules are in [`rules/enforcement/`](rules/enforcement/), with their
candidates in `$D/rules/enforcement/`. V8's deprecations come from
`tools/cr/codehealth/scripts/v8_deprecations.py`, which reads `src/v8` across
the `v8_revision` range of `src/DEPS`.

#### f. Migration campaigns

The rules are in [`rules/migrations/`](rules/migrations/), with their
candidates, `campaigns.py`'s clusters of commits by `Bug:` id, in
`$D/rules/migrations/`. Large feature-bug clusters (Glic, Actor) are noise
unless Brave touches the code.

#### g. Obsolete Brave code

The rules are in [`rules/obsolete/`](rules/obsolete/), with their candidates in
`$D/rules/obsolete/`. Most come from `tools/cr/codehealth/scripts/obsolete.py`,
which indexes Brave's citations, patches, test filters and overrides; run a
check with `--brave origin/master` to see what the lift had to fix. Its checks
keep to the range, and only count what predates it; `--debt` lists those rows,
for a fresh tracker. Polymer overrides are sweep [d](#d-webui)'s; Java moves are
sweep [h](#h-android-and-ios)'s.

#### h. Android and iOS

The rules are in [`rules/android_ios/`](rules/android_ios/), with their
candidates in `$D/rules/android_ios/`. Feature-list and `about_flags` overlaps
are noise here; use the feature default diff from sweep
[b](#b-brave-disablements-and-defaults) instead.

#### i. Subject triage

The sweeps above cover most of what matters. The rules in
[`rules/triage/`](rules/triage/) catch the rest by subject: deprecations and
enforcement, migrations and clean-ups, moves and renames, memory safety and
crashes, launches and default enables. They are noisy, so skim the subjects and
open only those naming an area Brave customizes. The noise every group drops is
the `exclude_subject_expression` in
[`rules/triage/COMMON_METADATA`](rules/triage/COMMON_METADATA): ash, ChromeOS,
Fuchsia, Perfetto rolls, gardener test disables and test migrations. Keep a
platform-only subject anyway when a Brave override touches the same code.

### 3. Read the residue

The sweeps only find what they look for. Before writing items, list the commits
no sweep read:

```sh
awk 'NR == FNR { seen[substr($1, 1, 12)]; next }
     !(substr($1, 1, 12) in seen)' $D/claimed $D/subjects > $D/residue
```

Subagents report hashes of different lengths, hence the fixed-length prefix.

Drop the subjects that the `exclude_subject_expression` in
[`rules/triage/COMMON_METADATA`](rules/triage/COMMON_METADATA) matches, then
skim the remaining subjects. Open the `--stat` of any commit that names an area
Brave customizes, and read its diff as in sweep
[a](#a-overlap-commits-touching-what-brave-overrides). The residue is most of
the range, so split it across subagents by subject prefix or top-level
directory.

The residue rarely pays, so keep it short. What finds items there is skimming
subjects for flags and campaigns, and matching full paths of the upstream
headers and Java classes Brave uses. Matching basenames is noise: generic names
(`app.ts`, `README.md`), `BUILD.gn`, `DEPS`, `*.xml` and `features.*` make up
nearly all of its hits.

Items found here meet the same ground rules as any other. Each one is a
mechanism no sweep describes, so record it in the method notes for step 5.

### 4. Write the items

For each item, record:

- what changed upstream, and why it affects Brave (one or two sentences)
- the motivating upstream commit(s), linked
- one checkbox per unit of work: a call site, field, file, or class, each with
  its path, and a line number when it helps
- the replacement idiom, when upstream's migration commits establish one

The sweeps also find things that predate the range. List those as well, and say
which lift introduced them; no later review will find them otherwise.

### 5. Update this document

Before finishing, if the review showed this file, a rule or a tool to be wrong,
missing or no longer needed, change it, following
[Maintaining this document](#maintaining-this-document). Otherwise leave them
alone.

## Rules

A sweep's rules live in `rules/<sweep>/`, in the format of Chromium's
`agents/knowledge_base`, so that a reviewer reads one mechanism and the commits
it applies to, not the whole range:

- **`<rule>.md`**, one per mechanism: frontmatter `name: <sweep>/<rule>`,
  `rubric: <sweep>` and `severity` (`regression`, `deprecation`, `migration`,
  `obsolete` or `optional`: the tracker section its items go to), then
  **Rule.**, **Why.**, **How to check.** and `## Examples`, tagged by milestone
  as in [Adding a heuristic](#adding-a-heuristic).
- **`COMMON_METADATA`**, a textproto of `capability_groups`, each naming the
  rules it feeds. A commit is a candidate when a file it touches matches
  `path_expression`, and a line it changes matches one of the
  `content_patterns`. `sweep.py`'s docstring has the rest of the fields: an
  exclusion, files Brave overrides, names Brave mentions, required patterns,
  ast-grep patterns (C++ read through plaster's macro blanking, `.h` as C++, GN
  through plaster's grammar, JSON as JS), and tools such as `java_names.py`.

A pattern only has to find the commit; the reviewer finds the item. The tools,
`sweep.py` included, are in `scripts/`, each with its `_test.py`.

## Tracker format

```markdown
# M<milestone> code health follow-ups

Tasks found by reviewing upstream changes in `<range>` (<N> commits) for
regressions, deprecations and code-health migrations that apply to Brave. Tick
items off as they land.

## 1. Regressions

## 2. Upcoming deprecations

## 3. Code-health migrations

## 4. Obsolete Brave code

## 5. Optional

## For privacy review

## Tooling TODOs
```

Each numbered section holds `###` items, each with its explanation and
checkboxes. Leave out empty sections, and order items within a section by
urgency. Regressions go first: silent ones, then those only some platforms
build. "For privacy review" is a plain list, one line per upstream feature.
"Tooling TODOs" is filed by lift runs (see `tools/cr/autolift/SKILL.md`): keep
it when updating the tracker, and don't review it as upstream items.

Tick a task with the subject of the commit that fixed it, as
``- [x] <task> (fixed in `<subject>`)``. The older milestone's branch picks
fixes from a newer one when a minor bump cherry-picks the same upstream change
(`autolift.py next-tracker` finds them by that subject), and a subject survives
the rebases a hash doesn't (e.g.
``(fixed in `[cr159] Restore Brave's version name`)``). A subject that holds
backticks goes in double ones, ``` ``[cr159] Pin `kFoo` off`` ```, or Prettier
mangles it. A task closed without a fix says why instead.

Format the file afterwards with `pnpm run format --base HEAD`. Without
`--base HEAD`, it formats everything the cr branch changed since
`origin/master`. It skips untracked files, so run
`node_modules/.bin/prettier --write` on a new tracker.

## Updating a tracker

When the tracker already exists, for a new range or a re-check:

- Keep ticked items, and keep their notes.
- Re-verify the open items: if the code an item names is gone or already
  migrated, tick it and say so, with the subject of the commit that did it if
  there is one (see [Tracker format](#tracker-format)). If the item no longer
  applies, close it with a one-line reason, as done for evaluated-and-skipped
  items.
- Add new items under the right section. Don't duplicate an existing item;
  extend its checkbox list instead.
- Replace any `cr<milestone>` branch hash in the tracker with the commit's
  subject (`git log -1 --format=%s <hash>`; see the
  [ground rules](#ground-rules)).
- Update the range and commit count in the intro. Count with
  `git log --oneline <first old tag>..<new tag> | wc -l`; ranges don't add up.
- Brave's fixes for open items often land on `origin/master` (for example
  `[bedrock]` commits), not on the cr branch, so `origin/master..HEAD` misses
  them. Search `git log` for the item's files instead.
- An incremental range (a few days of Chromium) still needs the whole set of
  sweeps, since a mid-milestone lift lands migrations that earlier ranges only
  announced. Also match the tracker's linked hashes and subjects against
  `reverts` (`grep -E '(Revert|Reland) "'` over `subjects`, unanchored, as some
  subjects start with an emoji) and the bodies of the reverts: a revert or
  reland can change an item's premise.

## When fixing items

- Whole-file formatters can add unrelated churn (e.g. `InsertBraces` in Brave's
  clang-format config), so format changed lines only.
- `std::string` results of a removed API are often only an intermediate. Pass
  the bytes on directly (e.g. to `base::Base64Encode` or `base::HexEncode`)
  rather than converting back to a string.
- For enum ↔ string mappings, `//base` has no reflection. Use a constexpr
  `base::fixed_flat_map` for name → enum, and an `operator<<` (picked up by
  `base::ToString`) for enum → name.
- Polymer overrides of an element that became Lit move to a lit_mangler named
  after upstream's input (`foo.html.ts.lit_mangler.ts`). Hide nodes rather than
  removing them when the element's `$` interface declares them.
- When a fix restores a disablement, add a test that fails if upstream moves the
  code again, e.g. a browser test checking that the helper isn't created, or an
  entry in `app/feature_defaults_unittest.cc`.

## Maintaining this document

Change this file, the rules and the tools only when a review shows something in
them wrong, missing or no longer needed: a verified item no rule describes, a
miss, a broken tool, a list too noisy to read. A review that only followed them
leaves them alone: what it found goes in the tracker. Keep them short: replace
text rather than append to it, and cut what no longer pays off.

State what to do, not how a review came to need it: no account of what happened
in a review (counts, which sweep found what, how long something took). A rule's
`## Examples` are its evidence, the commits its patterns must keep listing: name
the mechanism in each, nothing more.

### Inputs

- **Method notes** from each sweep: which rules found verified items, and which
  lists were only noise.
- **Each verified item**, with the rule that found it. An item no rule describes
  is a missing rule.
- **Misses:** problems in an already-reviewed range that surfaced later, through
  a rebase, a bug report or a fix. Look in `src/brave` history for commits whose
  "Chromium changes:" link names an upstream commit an earlier tracker's range
  covered. They're the most valuable input.

### Fixing the tooling

Fix `sweep.py`, the tools and the patterns during the run, as soon as a problem
shows up: a crash, a wrong match, a list too noisy to read, a commit a list
should have had, or a check that would have found an item sooner. Don't work
around it or leave it for later. Add a test for the fix, and rerun the affected
sweep on a range with the rule's examples to check they're still listed.

Only put in the tracker's Tooling TODOs what needs someone's input: a change
whose direction you aren't confident of, written as the question and its
options. Notes on what a tool should do better are changes to make, not to file.

### Adding a rule

- Add only what found a verified item, or what would have caught a miss. A miss
  is usually a pattern that didn't list the commit: widen it before adding a
  rule.
- One rule per mechanism, under the sweep it belongs to (see [Rules](#rules)),
  with a capability group whose candidates include the example's commit. Create
  a sweep only for a new way Brave gets exposed.
- Keep it range-independent: file lists, line numbers and counts go in the
  tracker. Tag examples with their milestone, e.g. `M158, 8127.2:`.
- If a rule for the mechanism exists, refresh its example or sharpen its check
  instead.

### Removing a rule

- Delete a rule, its file and its name in the groups, as soon as it is known to
  be obsolete: its mechanism is gone from Brave (no Polymer overrides left), or
  tooling now enforces it (a presubmit check, a test, a runtime error). Leave a
  one-line pointer to that tool only if it helps a reviewer.
- Delete a rule whose newest example is three milestones old and that found
  nothing since.
- When a list only produced false positives, narrow its patterns (an exclusion,
  `brave_mentions`, `required_patterns`) and say in a `COMMON_METADATA` comment
  what the narrowing drops.
- When a rule's check is mechanical, turn it into a tool and point the rule at
  it.

Lessons about implementing fixes go under
[When fixing items](#when-fixing-items). Commit everything under
`tools/cr/codehealth/` as `git commit -m 'Version CodeHealth tracker'`.

# Autonomous Chromium lift

Instructions for an agent that keeps a `cr<milestone>` branch up to date with
Chromium, without a human to ask. The run:

1. resets the local branch to `origin/cr<milestone>`, or creates it for a new
   milestone;
2. rebases it onto its base (the previous milestone's branch, or master);
3. lifts it with 🚀Brockit, if there is a tag to lift to (see
   [Lift target](#lift-target));
4. builds Linux and fixes what breaks;
5. builds Android and fixes what breaks;
6. pushes the branch, if the run finished with no issues and changed it (for a
   new milestone, also opening its pull request).

A maintenance run that has nothing to lift to stops after the rebase, a sync,
the Linux build and the push (see
[Maintenance run without a lift](#maintenance-run-without-a-lift)). A run with
nothing worth rebasing or lifting becomes an autofix run, which only fixes the
branch's failing CI (see [Autofix run](#autofix-run)). A run given a Chromium
tag instead of a milestone bumps Brave's release channels to it (see
[Channel bump run](#channel-bump-run)).

Push only with `autolift.py publish` (see [step 11](#11-publish)), with
`--no-rebase` in an autofix run, and don't open PRs or comment on GitHub
otherwise (beyond what `brockit lift --with-github` and `publish` update). Keep
a running report (see [Report](#report)) and finish by printing it.

A dry run, asked for by the job, passes `--dry-run` to `prepare` (or `autofix`)
and does all the work, but writes nothing to GitHub: `lift` writes no version
issue, `publish` reports what it would push and stops there, and no CI check is
rerun. `status` says when a run is one. Once asked to publish what the dry run
made, run `publish` with `--end-dry-run`, rather than starting over; `publish`
then opens the version issue the dry run's lift didn't.

Commands below run from `src/brave` unless they say otherwise. `..` is `src/`,
the Chromium checkout, where the upstream history lives. Every command starts in
`src/brave`: CI returns the shell there after each one. Write one that runs from
`src/` as `cd .. && <command>`, and run the `tools/cr` scripts (`autolift.py`,
`brockit`, `plaster`) only from inside `src/brave`: they find the checkout from
the working directory, and fail from `src/`.

Run Python scripts and their tests with `vpython3`, never `python3`: the system
Python lacks their dependencies. Run Brockit and Plaster as `brockit` and
`plaster`, which are on the `PATH`, rather than through `tools/cr/brockit.py`
and `tools/cr/plaster.py`.

## Inputs

- **Milestone `N`** and the branch, `cr<N>`. The branch may not exist yet: for a
  new milestone (e.g. 159, when
  `brockit show --chromium-version-label @latest-tag` is `159.0.8128.3` and
  there's no `origin/cr159`), the run creates it.
- **Base:** `origin/cr<N-1>` while the previous milestone is still on its own
  branch, and `origin/master` once it has landed there. It is set as the
  branch's upstream (see [step 1](#1-reset-and-rebase)).
- **Lift target:** picked by `autolift.py prepare`, which prints it and records
  it for `autolift.py lift` (see [Lift target](#lift-target)).
- **Bump range:** printed by `brockit lift` as "Changes for this bump"
  (`158.0.8117.1..158.0.8127.2`). Culprits for the lift's own breakage are in
  this range. Brockit doesn't print it for a new milestone's first bump;
  `autolift.py lift` then reports the `package.json` versions of the upstream
  and `HEAD` (`158.0.8127.2..159.0.8131.1`).
- **Branch range:** "Changes since base version"
  (`157.0.8103.15..158.0.8127.2`). It covers every bump already on the branch;
  use it to verify culprit hashes and to look for earlier fixes. A maintenance
  run without a lift takes it from `prepare`'s "Branch range" line instead.
- **Tracker:** `tools/cr/codehealth/M<N>.md`, if it exists. Read it before
  starting: issues it already tracks are left alone (see
  [Ground rules](#ground-rules)).
- **Chromium tag**, for a channel bump run only (e.g. `157.0.8103.38`): `N` is
  its major, and each channel is its branch's base.

## Ground rules

These apply to every step. The reason is given so that edge cases can be judged.

### Scope

- Act only on Brockit sections marked **(action needed)**. Ignore its other
  warnings (e.g. "Patch being flagged as broken, but with unexpected reason",
  "pnpm run init is failing to reset some paths").
- Before fixing a non-trivial runtime or test failure, check
  `tools/cr/codehealth/M<N>.md`. If the issue is already tracked there, leave it
  and say so in the report.
- Only run tests relevant to the code a fix touches. Full suites are other CI
  jobs' business.
- Don't build while Brockit has open "action needed" items. The tree is
  half-patched and the build only fails at `gn gen`.
- Leave what is broken on the base branch: it is fixed there, and reaches
  `cr<N>` with the next rebase. Find the culprit before fixing any failure, in a
  lift, a rebase-only run or an autofix run alike. It is the base's when the
  base has the cause as it is: an upstream culprit that predates the branch
  range (in the base's Chromium tag:
  `git -C .. merge-base --is-ancestor <culprit> <base's tag>` succeeds), or
  Brave code no `<base>..HEAD` commit touches. Judge it from the culprit, not
  from the base's CI, whose state can differ for reasons the run doesn't know. A
  base change that breaks only with this branch's Chromium (its culprit is in
  the branch range), or that clashes with a `cr<N>` commit, is this branch's to
  fix.
  - Don't fix, filter or rerun it, and carry on with the run: list it in the
    report with its culprit and the base it belongs to, and leave it out of what
    the steps require (a build that is green but for it counts as green).
  - Stop only when it blocks the run, with nothing left to do around it (the
    build fails before reaching what the run must verify): don't publish, and
    report that the run waits for the base's fix.

### Decisions without a human

When a fix needs a judgement call that would normally go to a reviewer (several
viable designs, a behaviour change, new third-party deps, dropping a Brave
feature), pick the most conservative option that keeps the build going, commit
it with `--tagged=WIP`, and explain the concern and the alternatives in the
commit body and in the report. Then carry on.

### Commits

- Commit with `git cr commit`, never plain `git commit` (except the mangler
  snapshot, below, and fixes picked from a newer branch with
  `autolift.py pick-fix`, which keep their message). Pass `--culprit <hash>`
  whenever there is an upstream culprit. The hook writes the "Chromium changes:"
  block and the `[cr<N>]` prefix; don't hand-write either.
- Pass the culprit as a full 40-character hash obtained from
  `git -C .. rev-parse <short>` or copied verbatim from tool output. Never type
  or complete a hash from memory. The commit hook refuses a culprit that doesn't
  resolve, or isn't between the Chromium tags the branch's upstream and HEAD
  pin.
- One commit per culprit. If a commit for that culprit already exists on the
  branch, add to it with `git cr commit --fixup=<sha>`, or `--fixup=amend:<sha>`
  when its message must change too (for an `amend!` message, keep the original
  subject and the "Chromium changes:" block, or the squash loses them). Fixups
  are squashed by the next `brockit rebase`.
- Platform-centric fixes get the platform tag: `--tagged=Android` for Android,
  `--tagged=iOS` for iOS. Fixes that aren't ready get `--tagged=WIP`. Combine as
  `--tagged=Android,WIP`; a repeated `--tagged` keeps only the last. A `--fixup`
  takes no `--tagged` (`git cr` refuses it): it squashes into its target, whose
  tags stand.
- `git cr commit` commits everything staged, and `follow-renames` and `git mv`
  stage as they go. Check `git status --short` and `git diff --cached --stat`
  before each commit.
- A fix without an upstream culprit (a latent Brave bug that only now shows up)
  is committed without `--culprit`; say why in the body. Before concluding there
  is none, widen the search past the bump range: what builds on the previous
  milestone's branch but breaks here has its culprit in the branch range, which
  can predate this bump when a base change unmasks it (the rebase brought in a
  Brave commit that removed a path the code also relied on). If a branch commit
  already cites that culprit, the fix is a `--fixup` to it. Never commit "no
  culprit" from a bump-range search alone.
- Simple IWYU fixes, adding an include that a Brave file used to get
  transitively, don't need a culprit hunt. Collect them in one commit,
  `git commit -m 'IWYU fixes.'`, with no body, culprit or tags (the subject is
  pinned, so rebases squash repeats). A fix that needs more than the include (a
  new GN dep, a moved header, a `chromium_src` change) is an ordinary culprit
  fix.
- Updates to the upstream test exclusions in `test/filters/` go in a pinned
  commit too: `git commit -m 'Disable failing upstream tests.'` for entries
  added or retargeted (an upstream test that fails because of what Brave
  changes, one renamed upstream), and
  `git commit -m 'Remove dead upstream tests from filters.'` for entries whose
  tests upstream deleted. No culprit or tags; say why each entry is needed in
  the filter file's comment above it. Entries in `test/filters/` (not
  `generated/`) are grouped by reason: a group is one comment giving the reason,
  then its entries, with a blank line before the next group. Add an entry to the
  group whose comment gives its reason, widening the comment if needed ("This
  test" becomes "These tests"); with no such group, start a new one at a blank
  line between groups. Never put a comment between a group's entries: it splits
  the group, and the entries above it lose their reason. Nor add one under
  "Tests below this point have not been diagnosed": find its reason first. The
  exception is an entry added or removed because of a browser change in the same
  commit (a fix that makes a filtered test pass again, a Brave change that makes
  an upstream test fail by design): it stays in that change's commit, or its
  fixup, so the two land and revert together.
- A plastered patch regenerated from its unchanged yaml (`plaster apply <yaml>`
  putting a stale `.patch` back as the yaml makes it), at any step of a lift or
  an autofix run, is a `--fixup` to the latest "Apply-fixed 🩹 patches from ..."
  commit on the branch, even when another commit wrote the stale patch, unless
  the patch first appears after that commit (a fix picked from a newer branch
  that adds the yaml): fix up the commit that adds it, or autosquash replays the
  fixup before the file exists (modify/delete). Never fix up an "Update patches"
  or "Updated strings" commit: those are regenerated, and
  `--discard-regen-changes` drops them, fixups included. A fix that changes the
  yaml is an ordinary culprit fix.
- Messages follow `../agents/shared/skills/cl-description/cl-description.md`: a
  `[Component]`-style subject under 72 columns, counting the `[cr<N>]` and tag
  prefixes the hook adds, and a body wrapped at 72 that explains why. To fix a
  committed subject, add an `amend!` commit with
  `git commit --allow-empty -F <file>`: `amend! <old subject>`, a blank line,
  then the whole new message, prefixes included, and the original's "Chromium
  changes:" block. The hook wraps nothing, so a `-m` body stays one line: write
  the message wrapped, with `-F`. Wrap with
  `vpython3 ../agents/shared/skills/cl-description/scripts/wrap_lines.py <in> > <out>`:
  it takes the input's first line as the subject (dropping its final period),
  prints to stdout and drops trailing blank lines. It leaves a paragraph that
  starts like a trailer (`WIP: ...`) unwrapped, so don't start one with a
  `<Word>:`. Wrap only your own text, not the "Chromium changes:" block (it
  quotes upstream's message, URLs and all); append that after a blank line, and
  pass the result with `-F`. Leave out the `Bug:` line. Never add
  `Co-Authored-By` or any other trailer: on a fixup, a body becomes the squashed
  message, and the hook appends after it.
- In commit subjects and bodies, wrap code names in backticks: classes,
  functions, methods, members, features, files and paths, GN targets, flags and
  commands (e.g. "Pin the `kTtc` default" and "Follow the `TabContainerImpl`
  removal", not "Pin the kTtc default").
- Don't claim in a message what wasn't verified.
- Anything under `tools/cr/codehealth/` (the trackers, Tooling TODOs included,
  their `SKILL.md` and the tools there) is committed with the message
  `Version CodeHealth tracker`.
- Changes to this file, the other `tools/cr/autolift/` files and the CI package
  in `tools/cr/ci/` are committed with `Version lift updates` (see
  [Committing it](#committing-it)).

### Editing upstream code

- Make every file change with the Edit and Write tools, one edit per change,
  never with a script (`python3 -`, `sed -i`, `perl -pi`, a heredoc or `>` into
  a source file). That includes resolving conflict markers and editing patches,
  yamls and upstream files. CI approves the Edit and Write tools for anything in
  the checkout, so their edits skip the permission review. A command that
  rewrites files goes through that review instead, which can't see that a change
  only carries a Brave customisation onto new upstream code, and blocks it.
  Tools that write files themselves (`plaster apply`, `brockit`, `git`, the
  `pnpm run` commands) are fine. If the review still denies an action, don't try
  another way to get the same result: stop, and record it in the report.
- Upstream files (anything in `src/` outside `src/brave`) change only through
  plaster or a patch. Never run a formatter over them.
- Before writing or editing the run's first plaster yaml, read `docs/plaster.md`
  and `docs/plaster_dos_and_donts.md` in full yourself, with the Read tool: not
  a subagent's summary, a skim or a `grep`, and not just the part that seems
  relevant. The rules below add to them; they don't replace them. Read them
  again after a context summary drops them.
- Change a plastered file by editing its `rewrite/**.yaml` and running
  `plaster apply <yaml>`. Always pass the yaml; a bare `plaster apply` rewrites
  every patch. Don't run `plaster check` (it is a dry run of `apply`); check the
  regenerated `.patch` and `git diff` instead.
- Regenerate a raw patch (no yaml) with `pnpm run update_patches <path>`, scoped
  to that one file. Never run it unscoped during a lift: it rewrites hundreds of
  patches. When the upstream file ends up identical to upstream,
  `update_patches` leaves the stale patch alone; `git rm` it.
- Ignore `.patchinfo` files. They are gitignored metadata: don't inspect, edit
  or delete them.
- Plaster vs `chromium_src`: file-scope additions (includes, helpers, constants,
  types) belong in a `chromium_src` shadow, which declares them and then
  includes the upstream file. Plaster is for edits inside function bodies,
  switches, initializers and enums. Prefer a precise plaster over a
  `#define X X_ChromiumImpl` token rewrite, which silently rewrites every later
  occurrence.
- When a fix touches a `chromium_src` shadow that `#define`s into the upstream
  file it includes, consider moving that `#define` to plaster: in the fix when
  the move is simple, otherwise in a follow-up commit right after it, with
  `git cr commit --tagged=plaster`.
- Converting to plaster covers the whole `.cc`/`.h` pair (both patches and the
  `#define`s in both shadows), and goes by the plaster docs above and
  `plaster --help` (not `.claude/skills/plaster-from-patch`, which is outdated).
  Beyond them:

  - A token-rewrite `#define` rewrites every occurrence in the file, comments
    included. List them in the pristine file (`git -C .. show HEAD:<path>`),
    cover each one that matters, and confirm the rest don't.
  - Prefer a rewriter over a regex whenever it does the job, even if its output
    isn't indented quite right; file the flaw as a Tooling TODO rather than
    working around it.
  - Delete shadows that only existed for the old macros, but keep shadow lines
    that aren't rewrites (the Windows `#undef LoadAccelerators`).
  - List the platforms the build didn't cover in the report, and commit with
    `git cr commit --tagged=plaster`.

- Plaster `description`s read like a commit message: a subject saying what, a
  blank line, then why. They describe the code as it is for the next reader, not
  how it got there: an upstream flag that was removed or a hook that moved
  belongs in the commit message.
- Keep `set_feature_flag_default_state` / `BASE_OVERRIDDEN_FEATURE` pins even
  when upstream's default now matches. Remove one only if upstream deleted the
  feature, together with every other mention of its name: its entry in
  `app/feature_defaults_unittest.cc`, its `X("<Name>")` in
  `base/compile_overridden_features.inc`, and, for a Blink runtime feature, the
  json5 plaster, its `RUNTIME_FLAG` in
  `third_party/blink/renderer/platform/runtime_enabled_features_unittest.cc`,
  `WebRuntimeFeatures::Enable<Name>()` and `EnableFeatureFromString("<Name>")`
  calls (no build step checks those strings) and Brave's disabled origin trials
  (`chromium_src/third_party/blink/common/origin_trials/origin_trials.cc`).
  `git grep -w <Name>` before committing, with the feature's string name, not
  the C++ constant: `-w kEnterpriseReleaseNotes` doesn't match
  `X("EnterpriseReleaseNotes")`.
- Pin a Blink runtime feature that has a `base::Feature` (a
  `runtime_enabled_features.json5` entry without `base_feature: "none"`) with
  the json5 plaster's `set_blink_runtime_enabled_feature_state`. It marks the
  entry `brave_base_feature_overridden`, so the runtime flag follows Brave's
  state instead of its `status`. Add the name to
  `base/compile_overridden_features.inc` (the build fails without it), a
  `RUNTIME_FLAG` to `runtime_enabled_features_unittest.cc`, and an entry to
  `app/feature_defaults_unittest.cc`'s Blink runtime feature tests, and disable
  its origin trial if it has one. Keep `EnableFeatureFromString()` for entries
  with `base_feature: "none"`, which the plaster can't pin, and drop the string
  of a feature the plaster pins.

### Writing Brave code

- Before writing a fix, read the `docs/best_practices.md` topics it touches
  (`chromium-src-overrides.md` for shadows, `build-system.md` for GN,
  `frontend.md` for WebUI, the `coding-standards*.md` for C++, `android.md`),
  and check the result against them before committing.
- Check each fix against Chromium's review rules in
  `../agents/knowledge_base/rules/`, picked by each directory's
  `COMMON_METADATA`, plus `general/` for the language. Its paths never name
  `brave/`, so match the upstream path a Brave file stands for
  (`chromium_src/<path>` and `rewrite/<path>.yaml` to `<path>`, a patch to its
  file, `brave/browser/` to `chrome/browser/`). Treat `severity: should_fix`
  rules as review findings.
- If upstream code builds in Chromium but not in Brave, find the Brave
  discrepancy instead of plastering upstream. The usual leak is
  `-iquote //brave/chromium_src`: every quoted include of an upstream header
  resolves to Brave's shadow, so a Brave header can reach upstream translation
  units several includes deep. For name-lookup errors, look for a Brave nested
  namespace that shadows a global one. The build environment is another source
  (see [Build system and environment](#build-system-and-environment)). Confirm a
  suspected mechanism with a small standalone repro compiled with
  `../third_party/llvm-build/Release+Asserts/bin/clang++ -fsyntax-only` rather
  than a build round. Once the discrepancy is removed, delete any plaster that
  only worked around it: the yaml, its `.patch`, and the upstream file's changes
  (`git -C .. checkout <path>`).
- Don't add null checks for values that can't be null at the call site, even if
  existing Brave or upstream code does. Use `CHECK()` for a real invariant.
- Run `pnpm run format --base=HEAD` before every commit of Brave files: code,
  tooling, this document and the tracker alike. It formats only the changed
  lines of Brave files. Revert what it reformats in lines the base has, but keep
  its reflow of lines a branch commit added: the presubmit formats against the
  base and flags them.
- In a `chromium_src` header that must include something before the upstream
  header, put a comment line between the two includes: clang-format otherwise
  sorts the `<upstream>` include first.

### Long commands

- Run commands that take minutes in the background (the Bash tool's
  `run_in_background`): `autolift.py prepare`, `lift`, `lift --continue` and
  `publish`, `pnpm run sync`, `brockit regen`, the builds, the presubmit and
  long test runs. Then end the turn: you are notified when the command finishes.
  Don't poll it, `sleep`, or read its output in the meantime: every check is
  another paid turn. Give builds the longest background `timeout` (7200000 ms):
  the 10-minute default kills a cold build midway. After a kill, see
  [Machine state](#machine-state).
- Keep their output out of your context: send it to a log in `.autolift/cr<N>/`
  (`> .autolift/cr<N>/<name>.log 2>&1`), as the commands below do. Never let it
  print, and never pipe it through `tee`, `tail`, `head` or `grep`. A CI run
  streams every `*.log` there to its console as it is written.
  `autolift.py lift` writes Brockit's output to its own log there, and prints
  only its summary.
- Once it finishes, read only what you need from the log:
  `autolift.py build-summary`, `grep`, `tail -n`. Never the whole log.

### Machine state

- Never delete caches, out directories or anything outside the repo working tree
  (`~/.cache`, the siso cache, `out/`). If one looks corrupt, record it in the
  report and stop. A build killed mid-run can leave truncated blobs in siso's
  local cache, served back as empty outputs ("failed to parse depfile ... EOF").
  In `../out/<dir>/siso.INFO`, "failed to flush ... interrupt by signal" is only
  the kill; "read 0 bytes: unexpected EOF" means the cache is corrupt, and
  removing one depfile only moves the failure. Retry once; after that, stop and
  ask for the cache to be cleared.
- Run one build per checkout at a time. Before starting one, check that no
  earlier build is still running there:
  `pgrep -af "siso|autoninja" | grep "$(realpath ..)"`. A build from another
  checkout is harmless.
- `brave/build/config/siso/brave_siso_config.star` is hardlinked into
  `src/build/config/siso/`. Editing tools that write atomically break the link,
  and the build then reads a stale copy. After editing it, run
  `vpython3 build/hardlink_file.py build/config/siso/brave_siso_config.star ../build/config/siso/brave_siso_config.star`
  and check that both paths have the same inode (`stat -c '%i %n'`).

## Tooling

Rely on tools over inference wherever one exists or can be written: 🚀Brockit,
🩹Plaster, `git cr` (`commit`, `follow-renames`), the `pnpm run` commands,
`tools/recipes`, and `autolift.py` below. A tool's result is reproducible and
reviewable; a judgement call has to be re-made on every run.

When a step needs work that a tool could do, or a tool gets something wrong or
says too little, file a request under `## Tooling TODOs` at the end of
`tools/cr/codehealth/M<N>.md` (create the section if it's missing). Write one
`###` item per tool problem: what the tool does now, the cr<N> example that
showed it, and checkboxes for the changes requested. Extend an existing item
rather than duplicating it. These requests are reviewed separately, so don't
implement missing features or improvements during a lift, and don't let one
block the run. Commit them with `git commit -m 'Version CodeHealth tracker'`,
like other tracker changes.

The code health scripts and rules (`tools/cr/codehealth/scripts/`,
`tools/cr/codehealth/rules/`) are the exception: improve them right away, as
`tools/cr/codehealth/SKILL.md` asks, with tests, and commit them as
`Version CodeHealth tracker`. File a Tooling TODO for one only when you aren't
confident of the direction, and then write what needs input: the question and
the options, not a task list.

A hard bug is different: a tool that crashes, corrupts the tree or commits, or
produces a wrong result the lift can't work around. Fix it in the tool, with a
test in its `tools/cr/*_test.py` and the tool's tests passing (run them from
their directory with `vpython3 -m unittest <module>`; vpython has no `pytest`).
Commit the fix on its own with `git cr commit --tagged=<tool>` (e.g.
`--tagged=brockit` for `brockit.py` or `rebase.py`), never mixed into a lift
fix, and list it in the report. If the fix isn't small and safe, work around the
bug, file it as a Tooling TODO, and carry on.

`tools/cr/autolift/autolift.py` collects the checks done repeatedly by hand:

```sh
# Run or continue brockit lift to prepare's target, and summarise what needs
# attention (step 2).
vpython3 tools/cr/autolift/autolift.py lift
vpython3 tools/cr/autolift/autolift.py lift --continue

# Reset cr<N>, set its base as upstream, pick the lift target, and start
# brockit rebase (step 1).
vpython3 tools/cr/autolift/autolift.py prepare 158

# At the very end: pick what landed on GitHub meanwhile, and push (step 11).
vpython3 tools/cr/autolift/autolift.py publish 158

# Outcome, failed steps and distinct errors of a build log.
vpython3 tools/cr/autolift/autolift.py build-summary .autolift/cr158/build-linux.log

# Upstream commits in the bump range touching paths, or adding/removing a string.
vpython3 tools/cr/autolift/autolift.py culprits 158.0.8117.1..158.0.8127.2 net/http/foo.cc
vpython3 tools/cr/autolift/autolift.py culprits 158.0.8117.1..158.0.8127.2 --symbol kFooFeature

# Maintenance branch: fixes on newer cr branches for the bump's cherry-picks,
# and picking one over with this branch's culprit.
vpython3 tools/cr/autolift/autolift.py next-fixes 158.0.8127.2..158.0.8127.4 net/http/foo.cc
vpython3 tools/cr/autolift/autolift.py pick-fix <fix commit> <cherry-pick hash>

# Maintenance branch: newer trackers' items concerning the bump's cherry-picks,
# with their state and fixes, and picking a ticked item's fix (step 8).
vpython3 tools/cr/autolift/autolift.py next-tracker 158.0.8127.2..158.0.8127.4
vpython3 tools/cr/autolift/autolift.py pick-fix <fix commit>

# Autofix run: start from GitHub's cr<N>, read what failed in its pull
# request's CI, and publish the fixups without a rebase.
vpython3 tools/cr/autolift/autolift.py autofix 159
vpython3 tools/cr/autolift/autolift.py ci-failures 159
vpython3 tools/cr/autolift/autolift.py publish 159 --no-rebase

# Where the run is, and the step to carry on with; record a step as it ends.
vpython3 tools/cr/autolift/autolift.py status
vpython3 tools/cr/autolift/autolift.py step 3 --done '`brave:all` builds; 4 fixes'

# Channel bump: branch the channels off for the tag (step 1); `publish`,
# `autofix`, `ci-failures` and `status` take the tag too. On a lower
# channel's branch, pick the higher channel's fixes.
vpython3 tools/cr/autolift/autolift.py prepare 157.0.8103.38
vpython3 .autolift/tooling/cr/autolift/autolift.py channel-picks
```

The run keeps its logs and its report in `.autolift/cr<N>/` in `src/brave`
(git-ignored): inside the workspace, and outside `out/`, so that a clean rebuild
(`rm -fr ../out`) doesn't wipe them. Don't keep them in `/tmp`. `prepare` and
`lift` write there themselves; send build logs there too. `grep` may be aliased
to `ugrep`, which skips binary files and handles `--include` differently; use
Python for binary inspection.

## Procedure

Start every run, new or resumed, with
`vpython3 tools/cr/autolift/autolift.py status`, rather than looking around by
hand: it prints the branch against `origin/cr<N>`, any rebase, cherry-pick or
`brockit lift` in progress, uncommitted changes, running builds, the lift
target, the steps the current run has recorded, the latest logs, and the step to
carry on with.

Before starting an agent, the recipe runs `autolift.py advise`, which decides
from `plan`'s rules and the pull requests' checks whether the run lifts,
rebases, only fixes the CI, or has nothing to do (no agent starts), and writes
the agent's prompt. Change the prompts there. On a branch with newer `cr`
branches above it, a run that will lift or rebase first brings the tooling up to
date; an autofix run, or one not worth it, doesn't. `sync_tooling.py`, which
ships with the recipe, cherry-picks the newer branches' culprit-free
`tools/cr/`, `build/` and `tools/recipes/` commits (without the `M<NNN>.md`
trackers), declines those following a Chromium change this branch's Chromium
lacks, and an agent reconciles what it left for judgement.
`.autolift/cr<N>/sync-tooling.md` says what was picked, skipped, declined and
dropped; copy its lists into the report. `prepare` keeps those commits, the
rebase replays them, and `publish` pushes them with the run's own. This document
is the synced one. CI runs the recipes from master, so a picked recipe change
only takes effect once merged there.

Once a step is done, record it with
`vpython3 tools/cr/autolift/autolift.py step <n> --done '<outcome>'`, the
outcome in a line or two (for step 3: "`brave:all` builds; 4 fixes"), or
`--skipped '<why>'`. It writes the `## Step <n> done: <title>` heading `status`
reads; don't write those headings by hand.

### Resuming a run

A run that stopped before the end (an error, an API outage, the job being
stopped) can be resumed: the CI job then keeps the checkout, the out dir and
`.autolift/cr<N>/` as the run left them, and asks you to resume instead of
starting over.

- Don't run `autolift.py prepare` again: it resets `cr<N>` to `origin/cr<N>`,
  which drops the stopped run's commits.
- Carry on with the step `autolift.py status` names. It follows the
  [Maintenance run without a lift](#maintenance-run-without-a-lift) steps when
  `prepare` found no lift.
- Check what the interrupted step left first: `status` lists a rebase or
  `brockit lift` that stopped midway and uncommitted changes, but not conflict
  markers or a build or test log that ends abruptly. Finish that step, or redo
  it, before going on. Read the report in full, but the logs as anywhere else
  (see [Long commands](#long-commands)): `autolift.py build-summary`, `grep` or
  `tail -n`, never `cat`.
- Compare `git reflog` with the report's commit lists: the branch can be
  rewritten outside the run between sessions (teammates' fixes cherry-picked on
  top, the run's fixups dropped). Don't undo it; list it in the report, and say
  which steps it came after and so wasn't verified by.
- Subagents don't survive an interruption. If step 8 stopped with sweeps
  running, rerun them all on the existing dump directory (complete if `subjects`
  has the range's commit count), ignoring the stopped sweeps' scratch files.
- Append `## Resumed (<date>)` to the report, saying which step you picked up at
  and why.

### 1. Reset and rebase

```sh
vpython3 tools/cr/autolift/autolift.py prepare <N>
```

It resets `cr<N>` to `origin/cr<N>`, sets the base as its upstream, picks the
[lift target](#lift-target), syncs the checkout if its Chromium version doesn't
match the branch's tag, and starts `brockit rebase`. It prints the base and the
target: use that base wherever this document says `<base>`. If it refuses to run
(a rebase in progress, changes to tracked files, local commits `origin/cr<N>`
lacks), stop and report it rather than passing `--force`.

Brockit only regenerates a conflicted patch from its plaster when the Chromium
checkout is on the version rebased so far. Before a `brockit rebase` you start
by hand, check the committed version, `git -C .. show HEAD:chrome/VERSION`, not
the working copy, which Brave's patches rewrite with Brave's own version.

For a new milestone, with no `origin/cr<N>` yet, it creates `cr<N>` from the
base (for 159: from `origin/cr158`), sets the same upstream, and skips the
rebase, since the branch has nothing of its own yet. Go straight to
[step 2](#2-lift): `--to=@latest-for-branch` takes the major from the branch
name. The new branch stays local until [step 11](#11-publish).

The base is `origin/master` once master's Chromium version is on milestone
`N-1`, which is when the previous milestone has landed, and `origin/cr<N-1>`
before that. Brockit takes the branch's upstream as the base for both
`brockit rebase` and `brockit lift` (`--from-ref` defaults to `@upstream`).

#### Lift target

`prepare` picks it. `cr<N>` is the head branch, lifting to `@latest-for-branch`,
until `cr<N+1>` appears on origin. From then on it is a maintenance branch that
lifts only to a newer beta or stable release of its own milestone. A head branch
skips minor bumps of its tag (only the last number moves). With nothing to lift
to, `prepare` says "No lift needed", and the run takes
[Maintenance run without a lift](#maintenance-run-without-a-lift) after the
rebase. Don't override the target, or lift by hand to `@latest-for-branch` on a
maintenance branch.

A run with no lift is worth it only for a rebase over more than 20 base commits,
over any once the branch was last rebased more than 12 hours ago, or over any
when the branch conflicts with its base (what GitHub shows as conflicts on its
pull request; `prepare` lists the files). Otherwise `prepare` says "Not worth a
lift run" and exits 3 without rebasing, and the run carries on as an
[Autofix run](#autofix-run), from its step 1: the branch's CI is still worth
checking and acting on.

When `prepare` says "the base moved from … to …", the base had a bump of its
own, so the rebase dropped the regen commits ("Update patches", "Updated
strings", the `gnrt` run); without a lift, remake them with `brockit regen` once
the rebase is done.

**When the previous milestone lands.** The first run after `cr<N-1>` merges into
master moves `cr<N>` onto master. `prepare` picks `origin/master` by itself, and
says so in its output and the report ("Base moves to origin/master"), because
`origin/cr<N>` still forks from master as it was before the merge. In that
rebase:

- Only the branch's own commits are replayed: Brockit starts after the previous
  milestone's commits, which master now has. Expect more conflicts than usual:
  master's own changes from the previous milestone's cycle arrive all at once.
  Resolve them with the recipes below, against master's version of each file.
- `origin/cr<N-1>` may no longer exist. Use `origin/master` wherever this
  document says `<base>`.
- The branch range ("Changes since base version") now starts at master's
  Chromium version.
- The `cr<N>` pull request, if there is one, still targets `cr<N-1>`. Don't
  retarget it yourself; list it in the report for the CI job.

The rebase also autosquashes `fixup!` and `amend!` commits. Brockit resolves the
conflicts that have a fixed answer as it goes:

- a patch with a yaml is regenerated from its plaster;
- a `tools/cr/codehealth/M<N>.md` the base doesn't have takes the branch's last
  version;
- the "Update from Chromium" commit takes its own `projects.chrome.tag` and the
  newer `PinsListTimestamp`, when that is the file's single conflicting hunk;
- a raw patch the base changed only in its context takes the commit's version.

It stops on any other conflict, listing the commit and the files left. Resolve
them with the recipes below, `git add` them, and carry on with:

```sh
brockit rebase --continue
```

To drop the commit it stopped on, use `brockit rebase --skip`. Never use plain
`git rebase --continue` or `--skip`: they skip Brockit's handling of the squash
messages and its conflict rules for the rest of the rebase. When the rebase
finishes, Brockit lists what it resolved, by rule. Recipes for what it leaves:

- **`patches/*.patch` with a yaml:** Brockit leaves these when the yaml
  conflicts too, when the plaster no longer applies, or when the Chromium
  checkout isn't on the version rebased so far (it warns). Don't resolve
  anything in the patch. Resolve a conflicted yaml first: usually the newer
  design on the base branch supersedes the branch's version. Check that it still
  applies to the new source and covers what the branch's version did, then
  regenerate the patch with `plaster apply patches/<name>.patch`. The same patch
  may conflict again in a later "Update patches" commit; regenerate it again. A
  substitution that no longer matches because the source already has its change
  is usually a backport that has landed upstream: see
  [Backports that landed upstream](#backports-that-landed-upstream).
- **`tools/cr/codehealth/M<N>.md`**, when the base has a copy too: take
  `origin/cr<N>`'s version.
- **A `chromium_src` shadow the base changed, in a commit that moves it to
  plaster:** take the commit's side, carry the base's change into the yaml, and
  regenerate the patches with `plaster apply`.
- **A cr<N> fix that the base fixed itself**, or tooling the base picked from
  cr<N> and then extended (check with `git diff origin/<base> -- <paths>`): take
  the base's version. A commit left empty is dropped with
  `brockit rebase --skip`; one left partly applied gets an `amend!` that rewords
  it to what remains.
- **The "Update from Chromium" commit**, when Brockit leaves it because more
  than the version bump conflicts: in `package.json`, keep the commit's
  `projects.chrome.tag`. In
  `chromium_src/net/tools/transport_security_state_generator/input_file_parsers.cc`,
  keep the newer of the two `PinsListTimestamp` values with its "Last updated"
  comment, so pins never move back in time. Resolve the rest on the merits.
- **Raw patches** (no yaml) that the base changed beyond their context: merge
  Brave's lines by hand. Brockit lists the raw patches it took the commit's
  version of at the end of the rebase. Raw patches should be rare. A patch can
  look raw only at that point in history, because a later commit on the branch
  adds its yaml: after the rebase, check whether the branch tip has a yaml for
  each one. Convert each one that is still raw in a follow-up commit, as below.
- **A `fixup!` of a pinned group that touches files the group moves later**
  (modify/delete conflict): Brockit places such fixups right after the group's
  first commit. Keep HEAD's state for that step (and undo anything git's rename
  detection applied to the old path), then requeue the same `fixup <sha>` line
  after the group's last `squash` line in
  `$(git rev-parse --git-path rebase-merge)/git-rebase-todo`, before running
  `--continue`. Check afterwards that the squashed commit has the fixup's
  change.
- **A `fixup!` of a pinned group that conflicts in a file a later branch commit
  also changes**: autosquash moves the fixup ahead of the commit it was written
  on top of, so neither side is the intended result. Resolve to the fixup's
  intent on `origin/cr<N>`'s version (`git show origin/cr<N>:<path>`), not the
  base's, then check with `git diff origin/cr<N> -- <path>` once that later
  commit has replayed.
- **Lit mangler snapshot** (`tools/chromium_src/lit_mangler/__snapshots__/`):
  resolve and add. It is regenerated at the end.
- **Tooling under `tools/cr/`:** keep both sides, then run the tool's tests
  before `--continue`: the merge can drop one side's line outside the marked
  hunk (cr158: `forks_test.py` losing `import contextlib`).
- **Test filters and other files:** resolve on the merits. When both sides
  filter the same test, keep the broader filter, whichever side it is on.

A commit whose changes are already on the base branch is dropped automatically
("patch contents already upstream"). File a Tooling TODO for any recipe you had
to apply by hand that Brockit could have applied.

### 2. Lift

```sh
vpython3 tools/cr/autolift/autolift.py lift
```

It runs `brockit lift --infra-mode --with-github --to=<target>`, with the
[lift target](#lift-target) that `prepare` recorded (always with
`--with-github`; it refuses to run when `prepare` found no lift). It keeps
Brockit's full log in `.autolift/cr<N>/`, and prints (and appends to the report)
the bump and branch ranges and what needs attention: each "action needed"
section with its items, failed checks (❌) and errors (🔥). Read the log only
for detail. Brockit updates Chromium, reapplies patches with `--3way`, and stops
with "action needed" sections. Handle each section, then run:

```sh
vpython3 tools/cr/autolift/autolift.py lift --continue
```

`lift` exits non-zero whenever anything needs attention, including a failed
plaster check after `--continue`, which Brockit itself exits 0 on. A zero exit
means the lift is done. It also prints the error Brockit crashed with, and
`lift --continue` only resumes a lift whose Brockit output asked to be rerun
with `--continue`: after a crash there is nothing to resume, so find what the
crash skipped from the error and `git log`.

On a new major version, Brockit first triggers the CI pipelines that build the
Rust toolchain for the new tag, and watches them ("Triggering pipelines for Rust
toolchain"). No commit appears until they finish, which takes a long time: let
it run. This needs CI (Jenkins) credentials on the machine (`~/.jenkins.json`).
If a pipeline fails, the lift stops there with no commits, and there is nothing
to `--continue`. Stop the run: don't retry the pipeline or work around the
toolchain. Report which pipeline failed and its link, and leave the branch as
`prepare` left it. A later run starts again from step 1.

For each item, find the culprit first
(`autolift.py culprits <bump range> <path>`). On a maintenance branch, look for
the newer branches' fix next (see
[Minor bumps on a maintenance branch](#minor-bumps-on-a-maintenance-branch)).
Then fix:

- **Files that cannot be patched anymore: renamed or moved.** Run
  `git cr follow-renames <full culprit hash>`. It moves patches and yamls and
  updates C++ includes, GN labels and Java class names (ProGuard flags
  included), and moves Brave's Java subclasses of a class that changed package
  along with it, but not namespaces or GN target labels that changed with the
  move. Grep for the old package, path and symbol names in Java, C++, GN, DEPS,
  `test/filters` and `app/feature_defaults_unittest.cc`, and fix what remains.
  Check that the patch applied to the moved upstream file. When upstream checks
  in Lit templates (`foo.html` becomes `foo.html.ts`), the `.lit_mangler.ts`
  files move with them, and a patch's `<if expr>` wrappers must stay inside
  `getHtml()`'s template literal.
- **Files that cannot be patched anymore: deleted.** Find where the code went.
  When a class is folded into its base (cr158: `TabContainerImpl` into
  `TabContainer`), the Brave subclass rebases onto the new class. Every method
  the subclass overrides that is no longer virtual needs `make_virtual`, plus
  `add_friend` for the subclass. A `#define Old Brave` that swapped in the
  subclass now matches nothing, and the build gives no error. Replace it with a
  plaster on the factory (`std::make_unique<TabContainer>(` →
  `std::make_unique<BraveTabContainer>(`). Delete the old patches and yamls.
- **Plaster failed to fix patches.** Read what the substitution targets and what
  the culprit did to it. In order of likelihood:
  - the target moved: move the yaml (cr158: `kTtc` to `components/ttc`);
  - upstream did the same thing: delete the yaml and its patch (cr158:
    `ENABLE_DESKTOP_ANDROID_EXTENSIONS` became `IS_ANDROID`); when what Brave
    carried was a backport of that upstream CL, revert the whole backport (see
    [Backports that landed upstream](#backports-that-landed-upstream));
  - the feature is gone: drop that substitution and every other mention of its
    name (see [Editing upstream code](#editing-upstream-code));
  - the code changed shape: rewrite the substitution.
- **Manually resolve conflicts for.** Open the upstream file in `src/`.
  - _Minor_ (context drift, the Brave hook still has a home): resolve the
    markers and stop. `--continue` refreshes the patch, so don't regenerate or
    commit it.
  - _Non-trivial_ (the code the hook targeted is gone or moved): resolve it,
    rebuild the customisation on the new code, regenerate or delete the patch
    with `pnpm run update_patches <path>`, and commit with the culprit. Read the
    whole culprit, not just the conflicting hunk: it can change more that Brave
    relies on elsewhere.

After `--continue` (or the first `lift`, when nothing needed action):

- "❌ GitHub issue update." The lift is done; run
  `brockit update-version-issue --infra-mode` into a log once `gh` works again.
- "❌ Plaster check. Please investigate it." Run `plaster apply --all` (the one
  sanctioned unscoped use) and inspect the diff. Nothing changed: carry on
  (stale `.patchinfo` metadata is a likely cause). Unremarkable, sensible
  changes: a `--fixup` to the "Apply-fixed 🩹 patches from ..." commit, as in
  [Commits](#commits). Significant changes: their own commit with a culprit,
  `--tagged=WIP` if unsure. A yaml that fails ("Unexpected number of matches")
  is handled like "Plaster failed to fix patches" above, even when the old patch
  still applied with fuzz.
- "❌ GNRT rerun. Please investigate it." Brockit prints no error: rerun
  `vpython3 ./tools/crates/run_gnrt.py vendor` (then `gen`) from `src/` into a
  log in `.autolift/cr<N>/`. A `[patch.crates-io]` entry in
  `brave/third_party/rust/chromium_crates_io/Cargo.toml` whose upstream
  `vendor/` copy is gone ("does not contain packages matching") means upstream
  rolled a crate Brave's crates still need at the old version: drop the patch
  entry so `gnrt vendor` fetches the crate into Brave's own `vendor/`, let
  `gnrt gen` write `brave/third_party/rust/<crate>/<epoch>/BUILD.gn`, and point
  Brave GN deps at the `//brave/third_party/rust/...` target. A Brave-owned copy
  is a `--tagged=WIP` decision (cr159: `base64` 0.22 after the 0.23 roll). Once
  `vendor` and `gen` run clean with no diff, the lift is done; no separate
  `gnrt` commit is needed.

#### Minor bumps on a maintenance branch

A maintenance branch's bump only changes the last number of the version
(`158.0.8127.2..158.0.8127.4`). Upstream's release branch carries no development
of its own: its commits are cherry-picks from main, each with a
`(cherry picked from commit <main hash>)` trailer. The newer `cr` branches have
lifted past those main commits already, so whatever a cherry-pick breaks has
often been fixed on `origin/cr<N+1>` (or a higher branch). Look there before
fixing anything, in the lift and in every build and test step after it:

```sh
vpython3 tools/cr/autolift/autolift.py next-fixes <bump range> [<path> ...]
```

It fetches the newer `cr` branches and lists each cherry-pick in the bump range
(touching `<path>`, when given) whose main commit a newer branch's commit cites
as its culprit. A fix cites the main commit, or its own branch's cherry-pick, so
the hashes differ from the ones in this branch's range; `next-fixes` matches
them through the cherry-pick trailers, and through the `Change-Id` every
cherry-pick of a change keeps, which a fix's "Chromium changes:" block quotes.
`lift` prints the whole list for a minor bump, with the files each fix touches:
match each action item and ❌ check against it first, and pick a fix that
touches its file rather than working one out (cr157: a plaster check failing on
a backport that landed upstream, which `cr159` had reverted already). Run
`next-fixes` again with the paths of each build or test failure. A fix already
on `cr<N>` (picked by an earlier run) is marked as such.

Bring a fix over with:

```sh
vpython3 tools/cr/autolift/autolift.py pick-fix <fix commit> <cherry-pick hash>
```

It checks that the fix cites the cherry-pick's main commit, and runs a plain
`git cherry-pick` of it (an exception to committing with `git cr commit`). The
message stays as it is: its `[cr<N+1>]` prefix and its culprit, the main commit,
are fine on `cr<N>`, so don't reword them. On a conflict, it stops: resolve it,
`git add` the files, and run `git cherry-pick --continue`. The newer branch's
fix may rely on code `cr<N>` doesn't have (other upstream changes, or other
Brave commits): keep only what this bump needs while resolving, and say in the
report what was left out. A fix spread over several commits is picked one commit
at a time, in order.

Not every fix cites a culprit. Patch refreshes land in the newer branch's
"Update patches" commits, so for a patch conflict, compare with
`git show origin/cr<N+1>:<patch path>` before resolving it by hand. When no
newer branch has a fix, fix it as usual, with the cherry-pick's hash as the
culprit.

### 3. Linux build

```sh
pnpm run build --target=brave:all \
    --gn=enable_updater:true --gn=use_prebuilt_omaha4:false \
    > .autolift/cr<N>/build-linux.log 2>&1
```

Use exactly this command; never raw `autoninja`, `ninja` or `siso`. The out dir
is `../out/Component`. The build stops at the first failure, so each round fixes
what failed and rebuilds; the build is incremental, so rounds are cheap. Don't
add `--ignore_compile_failure`: a failure is cheaper to fix as soon as it shows,
and later errors often only follow from it. Loop:

1. `autolift.py build-summary .autolift/cr<N>/build-linux.log`. For a failed
   build it also prints the build's own output, trimmed: read that before
   opening the log, and go to the log only for what it cut.
2. Steps running in parallel may fail together. Group their errors by root
   cause, not by file: one header change can fail many objects.
3. For each group, find the culprit and the fix (see
   [Fix patterns](#fix-patterns)). On a maintenance branch, pick the newer
   branches' fix when there is one (see
   [Minor bumps on a maintenance branch](#minor-bumps-on-a-maintenance-branch)).
   Independent groups can be researched in parallel, by subagents that only
   research. Edits and commits stay in the main agent so they don't race.
4. Commit each fix with its culprit, re-run `pnpm run format --base=HEAD` on
   Brave files, and rebuild. For a changed upstream signature, `git grep` every
   Brave caller first and fix them together: the build stops at the first
   object, so each missed caller costs a round.

If a log ends with an error about `out/Component/build.guard`, two builds
overlapped. Make sure only one runs.

To check one fix quickly, build just the targets it touches with the same
command and flags, e.g. `--target=brave_unit_tests`,
`--target=chrome/browser/resources/settings:lint_ts` or
`--target=chrome/test/data/webui/tab_search:build_ts`. A full `brave:all` build
is still what closes the step.

A green build doesn't show a `chromium_src` `#define` whose target upstream
removed: it just matches nothing. Once the build is green, run
`vpython3 tools/cr/codehealth/scripts/define_reach.py <bump range>` (the branch
range, in a run without a lift) and treat a token whose reach fell to 0 as a
failure.

### 4. Linux tests

Once the build is green, run the tests that cover each fix, and add coverage
where a runtime fix has none.

```sh
vpython3 tools/recipes/runtest.py --build-dir ../out/Component --xvfb \
    <binary> --no-sandbox --gtest_filter='<Suite>.*'
```

The binary is a name relative to `--build-dir` (`brave_unit_tests`,
`brave_browser_tests`, `brave_cert_pinning_tests`, upstream `unit_tests`; tests
of `brave/components/` are in `brave_components_unittests`). Check that the
filter matched something: "Tests took 0 seconds" or `[0/0]` means it didn't
(fixture names are case-sensitive: `SpeedReaderBrowserTest`, not
`SpeedreaderBrowserTest`).

Finding coverage:

- Search Brave test files for the class or function a fix touches. Feature
  default pins are covered by `FeatureDefaultsTest.*` in `brave_unit_tests`.
- Brave tests of overridden upstream code live at the matching
  `chromium_src/.../*_unittest.cc` path and are listed in `test/BUILD.gn`'s
  `brave_unit_tests` sources. A `chromium_src` shadow of an upstream unittest
  can include it and reuse its fixtures and helpers.
- A Brave suite added to an upstream WebUI mocha file through a `chromium_src`
  shadow (`chrome/test/data/webui/settings/*_test.ts`) runs from a Brave browser
  test calling `RunTest("<file>.js", "runMochaSuite('<suite>')")`
  (`browser/ui/webui/settings/brave_settings_toggle_browsertest.cc`), not from
  upstream's fixture: `--gtest_filter` on that one matches nothing.
- A new test must fail without the fix. Assert behaviour, not a value you just
  set; a test that fails with the fix may be showing the fix is wrong. For a
  plastered fix, check this directly: remove the plaster's lines from the
  upstream file, rebuild the test target and run it, then restore them with
  `plaster apply <yaml>` and check that the `.patch` is unchanged.
- Some code only builds in official builds (e.g. `enable_crash_dialog`). Its
  tests can't run in a Component build: write them, and list them as unverified
  in the report.
- Commit tests as a `--fixup` to their fix's commit.

Known environment-only failures, not lift regressions:

- `BraveFontWhitelistTest.*` fails on Linux distros that `GetLinuxDistro()`
  doesn't recognise (e.g. Ubuntu 26.04).
- Tests that pass on retry are flaky; report them, don't fix.
- "Missing X server or $DISPLAY" at start-up, every test failing in `SetUp()`:
  Xvfb wasn't up for that run, as for a test run chained after another in one
  command. Rerun it in a command of its own.
- An upstream test that checks behaviour Brave disables may already be in
  `test/filters/*.filter`. Check before investigating.

### 5. Lit mangler snapshot

```sh
pnpm run test-unit -t "mangled files should have up to date snapshots" -u
git add tools/chromium_src/lit_mangler/__snapshots__/mangle.test.ts.snap
git commit -m 'Update Lit mangler snapshot.'
```

Plain `git commit`, no culprit. The hook adds the prefix. Don't chase culprits
for individual stale snapshots. Run it again at the end if later fixes touched
manglers, and after `publish`'s rebase. If a rebase drops the snapshot commit as
"patch contents already upstream" and the test fails again, a commit outside the
snapshot's pinned group carries a stale hunk of it
(`git log origin/master..HEAD -- <snap>`): commit the update as a `--fixup` to
that commit instead.

### 6. Android

Only for a run that lifted. A run that only rebased builds Linux alone (see
[Maintenance run without a lift](#maintenance-run-without-a-lift)), and an
autofix run builds Android only for a failure in an Android check.

The checkout has Android's deps already: the recipe's `.env` lists `android` in
gclient's `target_os` for every sync, so build it without syncing again.

```sh
pnpm run build Static --target_os=android --target_arch=x64 \
    --target=brave:all --gn=android_static_analysis:on \
    > .autolift/cr<N>/build-android.log 2>&1
```

Use `x64`, so that tests can run on an emulator (see below for what it needs).
`android_static_analysis=on` runs Android lint, error-prone and the bytecode
checks as blocking build steps, so their findings fail this build; by default
(`build_server`) they run in a daemon and don't block. The out dir is
`../out/android_Static`. The build starts with `gn gen --check`, which catches
deps that a Linux-only `gn check` missed; when a lift changes GN deps, expect
Android to need its own. A "GN build args with no effect" line in
`build-summary` names an arg Brave sets that upstream removed; the build stays
green without it, so treat it as a failure. Like the Linux build, it stops at
the first failure. After changing `brave/DEPS`, run `pnpm run sync`, which also
checks that the new deps sync. All Android fixes get `--tagged=Android`.

For tests, run only the Android tests for code touched by Android fixes.
Robolectric tests run on the host, with `brave/script` on `PYTHONPATH`:

```sh
PYTHONPATH=$PWD/script ../out/android_Static/bin/run_brave_junit_tests \
    -f '*<Suite>*' > .autolift/cr<N>/test-junit.log 2>&1
```

The whole suite takes under a minute. Instrumentation tests, such as
`BytecodeTest`, need an emulator: `tools/android/avd/avd.py install` and `start`
with an x64 config such as
`tools/android/avd/proto/android_34_google_apis_x64.textpb`, run from `src/`
with `PYTHONPATH=$PWD/brave/script` (catapult imports `brave_chromium_utils`).
That downloads several GB and needs read/write access to `/dev/kvm`: check
`id -nG` for `kvm` before installing. Don't change group membership. If no
device can be had, list the Android tests as unverified in the report.

### 7. Presubmit

Once the Linux and Android builds pass, run the presubmit against the branch's
upstream (the base):

```sh
pnpm run presubmit --base "$(git rev-parse --abbrev-ref '@{upstream}')" \
    > .autolift/cr<N>/presubmit.log 2>&1
```

It checks every commit on the branch since the base, but only what is committed:
run it after the last commit, this document's update included. Commits the run
didn't write count too: the tooling `sync_tooling.py` picked, teammates' picks
and the rebase. `publish` runs it again before any push, and pushes nothing
while it has errors.

Address what it reports, errors above all, rebuild the targets a fix touches,
and run the presubmit again until no errors are left. Address warnings too when
they point at a real problem. Some warnings aren't worth addressing, or are
outdated: the one about empty lines in patches no longer matters, as plaster
generates those patches, and Chromium's "forward declaration is no longer
needed" counts uses in the shadow alone, not in the upstream header it includes
(cr159: `confirm_infobar_creation_allowlist.h`, whose plaster adds qualified
friends). Its "Includes STL header(s) but does not reference std::", an error
here, has the same blind spot for a shadow's include that serves plastered code;
it accepts any line with `std::`, so name the type in the comment above the
upstream include (cr159: `java_script_content_world.h`). For a `chromium_src`
override of a generated file, it asks for `check_chromium_src.py`: run it as
`PYTHONPATH=$PWD/script vpython3 tools/chromium_src/check_chromium_src.py Component`
(without the path it fails on `brave_chromium_utils`), into a log. List each
warning left alone, and why, in the report.

Then run the build scripts' checks (`tsc`, `eslint` and `jest` over `build/`),
with the JUnit report in the run directory rather than an untracked `junit.xml`:

```sh
JEST_JUNIT_OUTPUT_DIR=.autolift/cr<N> pnpm run test:scripts --verbose \
    --reporters=jest-junit --reporters=default \
    > .autolift/cr<N>/test-scripts.log 2>&1
```

Fix what fails like a presubmit finding, below, and run both again until they
pass.

A finding is almost always in code that a commit on the branch added, so fix it
in that commit, as a fixup. Find the commit that last touched the flagged line:

```sh
git log --no-patch --format='%H %s' -1 -L<line>,+1:<file> <base>..HEAD
```

and commit the fix with `git cr commit --fixup=<sha>`, using the commit a
`fixup!` fixes when the one found is itself a fixup. A fix is a commit of its
own only when no branch commit introduced the problem: the line is unchanged
from the base, or upstream changed what makes it wrong, in which case it gets
that culprit.

### 8. Code health review

Only a run that lifted reviews anything; a pure rebase (see
[Maintenance run without a lift](#maintenance-run-without-a-lift)) skips this
step. What the review is depends on the lift:

- **Head branch** (lifted to `@latest-for-branch`): the full review below.
- **Maintenance branch** (a minor bump to a beta or stable release): no full
  review. Check the newer branches' trackers instead (see
  [Minor bump review](#minor-bump-review)).

Once the presubmit is addressed, run `tools/cr/codehealth/SKILL.md` over the
range the tracker doesn't cover yet: from the end of the range in its intro (or
the base's tag, for a new tracker) to the new tag (`158.0.8127.2..159.0.8131.1`
for a first lift, `159.0.8135.1..159.0.8136.1` for a later bump). Write its dump
to `.autolift/cr<N>/codehealth_<new tag>`, not `/tmp`, so a resumed run can
reuse it. It writes or updates `tools/cr/codehealth/M<N>.md` with what fails
silently: overrides that stopped applying, re-enabled features, Brave copies
that missed upstream fixes. For a branch that isn't on origin yet, "what the
rebase already fixed" is `<base>..cr<N>` locally, not
`origin/master..origin/cr<N>`. Follow its own instructions (subagents, static
verification only), and commit what changed under `tools/cr/codehealth/` as
`Version CodeHealth tracker`.

Its sweeps run for an hour or more each; start them, end your turn, and wait.
The review is finished only when every subagent has sent its final result (a
notice that it still has background work is interim) and none is listed as
running. Never stop a sweep or write the tracker without one to finish sooner,
and don't publish before every result is in the tracker. Tell each sweep not to
leave commands in the background, and check `pgrep -af 'git grep|rg |python'`
for strays before writing the tracker.

Don't fix what the review finds, regressions included: the tracker is where
they're picked up, so list them in the report and carry on. A run that lifted
doesn't publish without it.

#### Minor bump review

A minor bump's changes are cherry-picks of main commits that the newer branches
have lifted past and reviewed already, so their trackers
(`tools/cr/codehealth/M<N+1>.md` on `origin/cr<N+1>`, and so on) are where any
concern about them is written down. This review is what makes sure the bump
didn't bring in a regression a newer branch has already flagged. Once the
presubmit is addressed, run:

```sh
vpython3 tools/cr/autolift/autolift.py next-tracker <bump range>
```

It lists each newer tracker's `###` items, and a `##` section's bullets before
its first `###`, Tooling TODOs aside, that concern the bump, with the matching
bullet (`in:`) and why:

- `cites`: the item cites the main commit of a cherry-pick in the bump, through
  the same trailers as `next-fixes` (cr157's `157.0.8103.15..157.0.8103.30` bump
  picked the change behind M158's "`IsChromeUntrustedDataSource` copy builds a
  static string").
- `names`: the item names a file a cherry-pick in the bump changed, by path or
  by file name (the same bump changed `feature_constants.cc`, which M158's
  "Android: page info moves into the app menu" names).

Read each listed item in full; don't read the rest of the tracker. A `names`
match can be incidental (a widely shared file): decide from the item whether the
cherry-pick touches what it describes. Each item also has its state (`open`,
`ticked`, `partly ticked`) and, for ticked tasks, the newer branch's fix: a
`fix` line when the task records the subject it was fixed in (see
`tools/cr/codehealth/SKILL.md`), or up to five `candidate` lines, the commits
that changed a file the item names, when it doesn't. For each item of notice:

- Check that it applies to `cr<N>`: the cherry-pick may carry only part of the
  main commit, and the Brave code the item names may differ on `cr<N>`.
- If the newer branch has fixed it, pick the fix:
  `autolift.py pick-fix <fix commit>`, without a culprit, as the fix answers the
  item rather than citing one (or with the cherry-pick's hash, when `next-fixes`
  lists the fix too; see
  [Minor bumps on a maintenance branch](#minor-bumps-on-a-maintenance-branch)).
  Pick a `candidate` only once `git show --stat` shows it does what the item's
  ticked task says; none may.
- If it isn't fixed, partly or at all, add what's left to
  `tools/cr/codehealth/M<N>.md`, citing the cherry-pick's hash, and commit it as
  `Version CodeHealth tracker`. Don't fix it.
- List it in the report, with what was done.

### 9. Update this document

Before finishing, if the run showed an instruction here to be wrong, missing or
no longer needed, change it, following
[Maintaining this document](#maintaining-this-document), and commit it.
Otherwise leave the file alone.

### 10. Wrap up

- `git status` is clean. Untracked files are scratch you created, or deps
  fetched into git-ignored paths.
- No build is left running.

### 11. Publish

A run that finished with no issues ends by pushing `cr<N>`. No issues means
every step the run takes is done: the lift, the Linux build, and for a run that
lifted, the Android build, a presubmit with no errors, passing
`pnpm run test:scripts` and a complete, committed code health review (every
review subagent back), and a clean wrap-up. `--tagged=WIP` commits don't hold
the push back: they're for review. If anything else is left, don't publish:
report why, and leave the branch local.

```sh
vpython3 tools/cr/autolift/autolift.py publish <N>
```

Run it in the background, with its output in `.autolift/cr<N>/publish.log`. It
picks what teammates pushed to `cr<N>` during the run, catches up with the base
once, runs a last `brockit rebase` and `brockit regen` when they would change
anything, runs the presubmit against the base over the branch it is about to
push (into `.autolift/cr<N>/presubmit.log`), and appends what it did to the
report, ending with "To review" links once it has pushed. For a new milestone,
it also opens the version issue and pull request.

- **Exit 1 with presubmit errors:** nothing is pushed. Fix them as in
  [step 7](#7-presubmit), and run `publish` again.

- **Exit 2:** it changed the branch and didn't push. Sync (`pnpm run sync`),
  rebuild with [step 3](#3-linux-build)'s command, fix what broke as in step 3,
  then run `vpython3 tools/cr/autolift/autolift.py publish <N> --built`. That
  round may ask for another sync and rebuild if more landed.
- **A conflict** in a pick or the rebase: resolve it like a rebase conflict,
  `git add` the files, run `git cherry-pick --continue` (or
  `brockit rebase --continue`), and run `publish` again.
- **GitHub's `cr<N>` no longer contains where the run started** (someone rebased
  it), or the push is rejected because something landed: nothing is pushed. In
  the first case stop and report; in the second, run `publish` again.

Then print the report.

### Maintenance run without a lift

When `prepare` reports "No lift needed" (see [Lift target](#lift-target)), on a
maintenance branch or on a head branch already on its latest tag, the daily run
only checks that the rebase onto the base broke nothing, with the Linux build
alone. After step 1:

1. Skip step 2: `autolift.py step 2 --skipped 'no lift'`.
2. Sync the Chromium checkout and Brave's patches to the rebased branch, in the
   background. If `prepare` said the base moved, the rebase ran with
   `--discard-regen-changes` and dropped the regen commits, so regenerate them;
   `brockit regen` runs `pnpm run init` itself, so there is nothing else to
   sync:

   ```sh
   brockit regen --infra-mode > .autolift/cr<N>/regen.log 2>&1
   ```

   Otherwise, sync:

   ```sh
   pnpm run sync > .autolift/cr<N>/sync.log 2>&1
   ```

   The Chromium tag doesn't change, but the rebase brings in the base's changes
   to `DEPS` and patches. A patch that no longer applies is fixed with the
   recipes in [step 1](#1-reset-and-rebase). `brockit regen` commits "Update
   patches", "Updated strings" and the `gnrt` run itself; check with
   `git log -3 --stat` that they hold only regenerated files.

3. Run [step 3](#3-linux-build) as usual. Breakage here comes from the base, not
   from a bump: new code on the base that doesn't fit `cr<N>`'s Chromium, or a
   base change that clashes with a `cr<N>` commit. Fix a clash in the `cr<N>`
   commit it clashes with, as a `--fixup`. Otherwise, when a Chromium commit in
   the branch range is what the base's code doesn't account for, name it with
   `--culprit`; when not, the base is broken on its own: leave it, as
   [Scope](#scope) says.
4. If the build needed fixes, run [step 4](#4-linux-tests) for them.
5. Run [step 10](#10-wrap-up).
6. Run [step 11](#11-publish): the rebase alone changes the branch, so it is
   pushed.

Then stop. Android, the Lit mangler snapshot and the code health review are for
runs that lift. Run the presubmit (see [step 7](#7-presubmit)) before step 11
whenever the branch differs from GitHub's: the rebase and the picked tooling
make it so even with no fix of the run's own. Step 9 applies as in a lift; do it
before step 11, so it is pushed too.

### Autofix run

A run turns into an autofix run when `prepare` finds nothing worth a lift run
(exit 3, see [Lift target](#lift-target)), and a job can ask for one. It only
fixes what fails in the CI of `cr<N>`'s pull request, on the branch as GitHub
has it: no lift and no rebase, so the commits on GitHub stay as they are, and
fixes go on top as fixups, which the next lift's `brockit rebase` squashes. The
ground rules apply as in a lift. Steps 1 to 11 don't; record what you do in the
report, not with `autolift.py step`.

1. Start, in the background:
   `vpython3 tools/cr/autolift/autolift.py autofix <N> > .autolift/cr<N>/autofix.log 2>&1`.
   It resets `cr<N>` to `origin/cr<N>`, keeping the tooling `sync_tooling.py`
   picked, sets the base as its upstream, and syncs a checkout on another
   Chromium version. If it refuses, stop and report it. A resumed autofix run
   doesn't run it again (it would refuse the run's unpushed fixups); carry on
   from the report.
2. Read the CI: `vpython3 tools/cr/autolift/autolift.py ci-failures <N>`. Exit
   0: every check passed. Exit 3: nothing failed, but some checks are still
   running, left to a later run. Either way there is nothing to do: end the run
   there, printing the report. Don't publish, rerun or wait for the pending
   checks; the tooling the run picked is picked again by the next run. Exit 2:
   some failed. It prints a line per failed check and writes the detail to
   `.autolift/cr<N>/ci-failures.md`: the builds behind each check (a TeamCity
   aggregate's failed dependencies, a Jenkins run's first failed stage and its
   unstable test stages, or its first aborted stage and why it was aborted, a
   GitHub Actions job's failed steps), with their problems, failed tests and the
   end of the failed step's log. Read that file, not the CI logs, and the logs
   only for what it cuts. `vpython3 tools/cr/ci/cli.py checks <N>` lists every
   check.
3. Triage each failed build. Its `infra`, `tests` or `build` label is a hint
   from the log; check it against the log tail.
   - **Infra** (the CI machine or its workspace: disk, agent, a workspace built
     another way): no commit. Rerun the check once with
     `vpython3 tools/cr/ci/cli.py rerun <check URL>`, adding `--wipe-workspace`
     on Jenkins when the workspace is the problem. TeamCity has no wipe through
     the CLI yet: when a plain rerun can't clear the workspace, report it for
     the CI owners (cr159: Android's "Run gn clean before switching from ninja
     to siso").
   - **Tests**: a test failing on several platforms, on every retry (TeamCity's
     "2 of 2 runs"), or since earlier builds ("failing since build", "failing
     for N builds") is broken, not flaky: fix it, unless its "History" lines
     show it passing in most of the branch's runs, which makes it flaky. A new
     failure on one platform that a rerun might pass is flaky: rerun the check
     once, and list the test in the report; don't fix or filter it. Muted
     TeamCity tests are already left out. A Brave test (in a `brave_*` binary)
     is fixed, never filtered to get CI green.
   - **Flaky upstream tests**: an upstream test that fails as a flake does (a
     timeout, a simulated input that never lands) has already failed the test
     launcher's retries, so it is filtered rather than rerun. Read its full
     output first (the Jenkins `testReport`, with its `age`), then check it
     upstream, on the platform that failed:
     `vpython3 tools/chromium_tests_analysis/is_upstream_flake.py <Suite.Test> <filter>`,
     with `--os Ubuntu`, `--os Windows`, `--os Mac`, `--builder android` or
     `--builder ios` as the filter. It answers by Chromium's own flake and
     failure criteria, for each variant, and exits 0 when one is confirmed
     upstream. The newest `cr<N>`, whose Chromium is still main's, passes
     `--ref main`. Any other branch checks with no `--ref`, which answers for
     the Chromium branch its checkout is on, and the closest picture of what it
     runs; then with `--ref main` too, which runs far ahead of that branch but
     can still show the test flaking, or fixed since. A branch without it has
     `check-upstream-flake.py <Suite.Test>` instead: a variant over the
     generated lists' threshold (1% over 30 days) counts as confirmed. One
     confirmed upstream goes in
     `test/filters/generated/<binary>-<platform>.filter`, edited by hand in that
     file's format, as the lists lag behind. One stable upstream has a Brave
     cause to name in `test/filters/<binary>-<platform>.filter` (cr157:
     `AutofillCounterTest.TimeRanges` hangs as it does on Linux). A Glic test
     goes in `<binary>.filter`, for every platform: Brave doesn't ship Glic.
   - **Upstream tests** (a Chromium binary: `unit_tests`, `browser_tests`,
     `components_unittests`, ...): either Brave broke what the test checks,
     which is a fix, or the test checks something Brave changes or disables,
     which is an entry in `test/filters/<binary>[-<platform>].filter`. Start
     from the test's entry under "Leads" in `ci-failures.md`: for each failed
     test, `ci-failures` lists the `tools/cr/codehealth/M<N>.md` items and
     filter entries that name its suite (wildcards included), its fixture's base
     classes, or a fixture the range renamed into it, and the commits that last
     changed those filters. The tracker records the filters that stopped
     matching: upstream renamed the suite or test, turned a `TEST_F` into a
     `TEST_P`, or added a subclass suite, and the test Brave disabled on purpose
     runs again (cr159: `ProfileReportGeneratorTest.SignedInProfile` became
     `SyncingProfile`). A parameter suffix can be an enum-set bitstring, which
     shrinks when upstream drops an enum value, with no tracker lead
     (`SetCookieFromStringCrossOrigin/SCRIPT_000100` became `_00010`); check the
     suite's `INSTANTIATE_TEST_SUITE_P` name generator. Then retarget the entry
     at the new name, as the tracker's task says, rather than adding one beside
     it, and tick that task. A test whose suite was filtered before is filtered
     again under its new name. With no lead, find why it fails before filtering
     it, and say why in the commit body. Act only on the tests CI reported: the
     tracker's other filter tasks wait for the lift, or for CI to fail on them.
     Unlike the other tracked issues, which the run leaves alone (see
     [Scope](#scope)), these are fixed: the tracker's task is the fix.
   - **Build** (a compile, `gn`, lint or storybook step): reproduce it with the
     narrowest local target, with the flags of [step 3](#3-linux-build)
     (`pnpm run build-storybook` for storybook), and fix it. Build Android, with
     the flags of [step 6](#6-android), only when the failure is in an Android
     check (cr159: `PR browser (android)`); every other failure is reproduced
     and verified on Linux, or fixed from the log for a platform that can't be
     built here.
   - **Broken on the base branch**, whatever the label: leave it, as
     [Scope](#scope) says, and carry on with the other failures.
4. Fix as in a lift, culprit first. Commit each fix as a fixup of the branch
   commit it corrects (`git cr commit --fixup=<sha>`; find it with
   `git log --format='%h %s' <base>..HEAD -- <path>`), or with a pinned subject
   (`IWYU fixes.`, the test exclusions' subjects as in [Commits](#commits), and
   `Version CodeHealth tracker` for the ticked task). For a stale plastered
   patch, that `git log` finds an "Update patches" commit: fix up the
   "Apply-fixed 🩹 patches" commit instead (see [Commits](#commits)). Only a fix
   with no commit to fix up (a culprit no branch commit cites) is a new
   `git cr commit --culprit`. Platforms that can't be built here (macOS,
   Windows, iOS) are fixed from the log, and listed as unverified.
5. Verify each fix locally: build the targets it touches and run the tests that
   failed, as in [step 4](#4-linux-tests), and with step 6's build and tests
   only for an Android failure. On a fresh out dir, build `brave:all` first: a
   narrow target misses Brave's paks, and browser tests then crash in
   `brave_component_loader.cc` ("invalid Brave Extension manifest") rather than
   where CI failed. Check filter entries by running the reported tests with
   `--test-launcher-filter-file` set to every filter that applies
   (`test/filters/` and `generated/`, `;`-separated) and checking that none of
   them runs: `--gtest_list_tests` ignores the filter file, so comparing lists
   shows nothing. Once fixes are committed, run the presubmit as in
   [step 7](#7-presubmit), and address its errors as fixups.
6. Publish without a rebase, in the background:
   `vpython3 tools/cr/autolift/autolift.py publish <N> --no-rebase > .autolift/cr<N>/publish.log 2>&1`.
   As in [step 11](#11-publish), it picks what teammates pushed to `cr<N>`
   during the run and pushes with a lease, but it doesn't catch up with the
   base, rebase or regenerate. Exit 2: the picks changed the branch, so sync
   (`pnpm run sync`), rebuild with [step 3](#3-linux-build)'s command, fix what
   broke as fixups, and run it again with `--built`. Presubmit errors stop it as
   in step 11. It refuses merges, and commits that aren't fixups, pinned
   subjects or the picked tooling: pass `--allow-new` when step 4 made a new
   commit. Conflicts and a rewritten branch are handled as in step 11. The push
   starts CI again; a later run reads the new results.
7. Report each failed check: its builds and label, and what was done (the fix
   commit, the rerun, or why it was left).

### Channel bump run

Given a Chromium tag (`157.0.8103.38`), the run bumps Brave's release channels
to it: master (nightly), and the beta and release `1.<minor>.x` branches below
master's Brave version, each one that is on the tag's milestone M<N> and behind
it. Each gets a branch off the channel, with the channel as upstream (`cr157.x`
off `origin/master`, `cr157.x-on-1.100.x` off `origin/1.100.x`), and a pull
request against it. The run keeps its report and logs in `.autolift/cr<N>.x/`.
The Procedure's steps apply, with these differences:

- **Tooling.** The channels don't carry this tooling. `prepare <tag>` copies
  `tools/cr` to `.autolift/tooling/cr/`, which checking out a channel leaves
  alone: from then on run `vpython3 .autolift/tooling/cr/autolift/autolift.py`,
  and read this file there. `brockit`, `plaster`, `git cr` and `pnpm` are the
  checked-out channel's own. There is no tooling sync. Step 9's changes are made
  in `.autolift/tooling/cr/` and committed as `Version lift updates` on the
  master channel's branch only (into its `tools/cr/`, written whole where master
  lacks the file); `channel-picks` leaves them out. No branch takes a
  `Version CodeHealth tracker` commit: what step 8 would add to a tracker goes
  in the report.
- **Step 1** is `prepare <tag>`. It creates the branches, with no rebase, and
  checks out the highest channel's. The names repeat from one bump of M<N> to
  the next: a branch an earlier bump left on GitHub is pushed over at step 11. A
  bump already on GitHub at the tag is rebased onto its channel when a `cr<N>`
  rerun would rebase (it conflicts with the channel, or is more than 20 commits
  behind it, or over 12 hours since its last rebase), and left as it is
  otherwise; `prepare` resets the one it rebases to GitHub's copy, and records
  each channel's action (`lift`, `rebase` or `none`), which `status` shows. Exit
  3: every channel's bump to the tag is on GitHub already, and none needs a
  rebase, so carry on as an autofix run of the tag (below). Exit 1 with "No
  channel is on M<N>": stop and report.
- **Step 2 on a branch to rebase** is `brockit rebase --squash-minor-bumps` with
  the checkout synced to it (`pnpm run sync`), resolved with the recipes in
  [step 1](#1-reset-and-rebase), then `channel-picks`: a channel left as it is
  still gives its fixes to those below it. A backport the channel took after the
  bump (cr157: the WebUSB backport, uplifted to beta and release) now needs the
  higher channel's revert of it. `publish` rebases again only if the channel
  moves once more.
- **Steps 2 to 5 and 7** run on the highest channel's branch, and `step` records
  them on the checked-out branch: `status` names the step and the branch to
  carry on with. Name logs after the channel
  (`.autolift/cr<N>.x/build-linux-master.log`). The lower channels take step 2
  alone, one at a time once the highest one's step 8 is done: the lift and
  `channel-picks`, with no build, tests or presubmit of their own, as the pull
  requests' CI and an autofix run cover them, unless there is a good reason to
  (say why in the report). There is no Android build (step 6) either. The
  checkout is synced to one branch at a time: after checking out another for
  anything but its lift (which syncs itself), run `pnpm run sync` before
  building, testing or checking it, or the checks read the other channel's
  patches (cr157: a false plaster error). `publish` and `autofix` sync each
  branch they check out to build or check. The lift is a minor bump: look for
  the newer branches' fixes as in
  [Minor bumps on a maintenance branch](#minor-bumps-on-a-maintenance-branch)
  first, on every channel.
- **On a lower channel**, run `channel-picks` once `lift` is done: it
  cherry-picks the higher channels' bump fixes this branch lacks, leaving out
  what each lift makes itself. Test exclusions
  (`Disable failing upstream tests.`) are picked all the same, though every
  channel has its own: only their changes tell them apart. It tries each first,
  and lists rather than picks those that would conflict or change nothing here,
  which usually concern what only the higher channel had: pick one by hand only
  when this channel's lift needs it. For a lift's action item, compare with the
  higher channel's branch first (`git show <higher branch>:<path>`), and resolve
  as it did where the code is the same. `next-fixes` there also looks at the
  higher channels' bump branches and the channels themselves. A fix the bump
  needs that is on master but not uplifted to the channel
  (`git log origin/<channel>..origin/master -- <path>`) is picked with
  `pick-fix` and listed in the report as an uplift.
- **Commits** on these branches get no `[cr<N>]` prefix and no culprit range
  check; `--culprit` still writes the "Chromium changes:" block. A fix only a
  lower channel needs goes on its branch alone, and on the channels below it
  through `channel-picks`.
- **Step 8** is the [Minor bump review](#minor-bump-review), once, after the
  highest channel's step 7, so that its picks reach the lower channels too.
- **Step 11** is `publish <tag>`. It brings every branch up to date with its
  channel, runs the presubmit of the branches the run builds (the highest
  channel's, and in an autofix run a lower one's with a fix of its own), and
  pushes none until all are ready. Then it pushes them all, highest first, and
  opens each pull request with `brockit update-version-issue` (unless one is
  open already), which sets the channel's labels and milestone. A channel
  `prepare` left as it is is pushed too, without a rebase, once the run commits
  on it. It leaves the branch it stopped on checked out: handle that as in step
  11 (for exit 2, which only the highest channel asks for: sync, rebuild it,
  then `publish <tag> --built`). With a git cache, `origin` fetches from the
  mirror: check what GitHub has with
  `git ls-remote "$(git remote get-url --push origin)"`, not `origin`.
- **Autofix run of a tag:** `autofix <tag>` resets each bump branch that is on
  GitHub at the tag, and `ci-failures <tag>` writes
  `.autolift/cr<N>.x/ci-failures-<channel>.md` for each pull request. Fix each
  channel's failures on its branch, highest first, building and testing the
  highest channel as usual. A lower channel that needs a fix the higher one made
  takes it with `channel-picks`, unbuilt: its pull request's CI checks it. Build
  and test a lower channel only for a fix of its own, for a failure the higher
  channel doesn't have; `publish <tag> --no-rebase` syncs, presubmits and asks
  for a rebuild of such a branch only.

## Fix patterns

Find the culprit first: the pattern tells you where to look, the culprit tells
you what upstream intended.

### Backports that landed upstream

Brave's master often carries upstream Chromium CLs ahead of their release,
cherry-picked as patches or plasters. Once the Chromium version a branch is on
has the CL itself, the backport conflicts: in a `brockit rebase`, when the cr
branch's tag already contains the CL the base backported, or in a lift, when the
new tag brings it. Typical signs are a plaster that fails with "Unexpected
number of matches" because the source already reads as the replacement, or a
patch whose hunks upstream already has.

- Find the backport: `git log -S'<the changed text>' -- patches rewrite` in
  `src/brave`, then its body or PR (cr159: "Backports https://crrev.com/c/…").
  Find the upstream commit that landed it in the range:
  `git log --grep='c/chromium/src/+/<CL number>' <old tag>..<new tag>` in
  `src/`.
- Revert exactly what the backport added, in one commit: everything
  `git show <backport>` adds to patches, yamls, `chromium_src` and anything else
  it touched, not only the file that conflicted, and nothing else in those
  files. Regenerate each affected patch with `plaster apply <yaml>`. If upstream
  landed only part of the CL, revert only that part.
- Commit it with the landing commit as culprit:
  `git cr commit --culprit=<landing commit> -m 'Revert the <feature> backport'`,
  naming the backport's hash and PR in the body.
- Keep it out of the "Update patches" commits: `--discard-regen-changes` drops
  those whole, and the revert with them. In the middle of a rebase, commit it
  after the rebase, or pass `--force-commit-hook`, as the hook otherwise leaves
  the message alone. When the rebase stops on the backport's patch in an "Update
  patches" commit, take that commit's side (`git checkout --theirs`) and
  continue: it is upstream's file without the backport, so the revert then
  changes only the yaml.

### Moves and renames

- **Namespace moves** (`kBrowserActuatorProtoStreamTransport` to
  `browser_actuator::transport`): update the qualified name, the include, the GN
  dep and the `DEPS` include rule.
- **Enum renames** (`WindowOpenDisposition::CURRENT_TAB` → `kCurrentTab`, no
  aliases): one mechanical rename across Brave in a single commit. Map every
  value, update comments that name the old values, then format. Check Java and
  TS separately (mojom Java bindings kept the old names). Code the base gains
  later still uses the old names: fix it as a `--fixup` to the rename commit.

### Signature and API changes

- **Brave overrides of a changed signature.** When a Brave `#define` declares an
  `_Unused` duplicate of an upstream method (`UpdateKeyPinsList`), update both
  declarations. If the new signature has a reply callback, run it, or callers
  waiting on the reply hang.
- **Classes replaced by free functions** (`SessionCrashedBubble` →
  `ShowSessionCrashedBubbleIfNotOffTheRecordProfile()`): a Brave `#define` on
  the old method name compiles nothing and silently drops Brave's hook (here,
  the crash report permission dialog). Re-route with a plaster at the call site,
  and declare the Brave wrapper in the `chromium_src` header.
- **Upstream deletes machinery a Brave feature relies on**, as the last user of
  a generic hook goes (cr159: `PermissionRequestDescription::requesting_origin`,
  which Brave Wallet keys account permissions on). Read why Brave used it before
  dropping the call. Restore only what Brave calls, as a defaulted virtual on
  the interface so the other delegates need nothing, implemented by the one
  class Brave reaches, and run the feature's browser tests. It is a
  `--tagged=WIP` decision.
- **Brave appends to an upstream enum after `kMaxValue`** (`DownloadCommands`):
  when upstream adds a value, renumber Brave's values past it and update the
  `static_assert`. Check first that Brave's values aren't persisted or recorded
  in histograms.

### Lost transitive includes

Upstream forward-declares something (`base::OneShotTimer`, `GURL`) and a Brave
file that relied on the old include chain breaks. Add the include to the Brave
file; don't plaster upstream. When adding the include is the whole fix, commit
it with the run's other such fixes as `git commit -m 'IWYU fixes.'` (see
[Commits](#commits)), without naming a culprit. When the fix needs more, or the
failure looks like more than a lost include, name the culprit: list the
translation unit's includes with a preprocessor run from the out dir (its
compile command from the log, `-c <src> -o <obj>` replaced by
`-E -H <src> -o /dev/null`, `-MMD -MF` dropped), and match them against the
headers that lost the include
(`git -C .. log <branch range> -G'#include "url/gurl.h"' -- '*.h'`).

### Build system and environment

- **Lost public include paths** (`gn check`: "Can't include this header from
  here" in a Brave target whose deps didn't change): a public chain to the
  header's target was cut. Find the commit that dropped it with
  `git -C .. log <branch range> -G'<header target>' -- '*BUILD.gn' '*.gni'`,
  then confirm the target reached the edited target publicly with a read-only
  query of the out dir, from `src/`:
  `buildtools/linux64/gn path out/Component <direct dep> <edited target> --public`.
- **Target turned into a `group`** ("Assignment had no effect" on `sources` at
  `gn gen`; cr159: `//chrome/test:test_support`): move the injected sources to a
  Brave target, or reuse one that already builds them, and add it to the group's
  `public_deps` with plaster `add_literal_to_list`.
- **Brave-only environment leaks**: an upstream action that works in Chromium
  but fails in Brave can be Brave's `pnpm run build` environment
  (`build/commands/lib/config.ts` putting a directory on `PYTHONPATH` whose
  modules shadow an upstream package's). Confirm by running the action's import
  with and without the variable. Fix at the source: drop the entry and make its
  last user add the path itself.
- **Upstream drops a library Brave uses** (Android-tools jars in the
  `android_deps` CIPD bundle): Brave's `DEPS` can declare per-library CIPD
  packages (`"dep_type": "cipd"`, `"condition": "checkout_android"`), fetched
  into a git-ignored directory. Look for them with
  `cipd ls chromium/third_party/android_deps/libs` and
  `cipd describe <package> -version latest`. Check the package layout with
  `cipd export` into a scratch directory. A new dependency source is a
  `--tagged=WIP` decision.
- **Windows-only `tools/cr` test failures**: a bare `Path.write_text()` writes
  CRLF there, which patch generation without `--ignore-space-at-eol` turns into
  whole-file diffs. Reproduce on Linux with a scratch script that defaults
  `write_text`'s `newline` to `'\r\n'`, run with
  `vpython3 -vpython-spec tools/cr/vpython.toml`, and pass `newline='\n'`.
- **Stale Brave workarounds**: when an upstream change fixes what a Brave
  workaround worked around, the workaround can become the bug (a `dex.py`
  plaster keeping a prebuilt out of intermediate dexing). Look for a Brave
  plaster or patch on the failing tool before suspecting upstream. Reproduce by
  replaying the action's exact command (from the `rule ... command =` line in
  the out dir's `.ninja` files), with its outputs redirected to a scratch
  directory: an output that matches the replay rules out a stale cache, and
  points at the command.

### WebUI

- **Shared files moved** (`cr_elements/web_ui_listener_mixin.js` to
  `/shared/settings/`): use the import path upstream's own code now uses, and
  look for side-effect imports (`md_select.css.js`) that type-checking won't
  flag.
- **Hand-written browser proxies replaced by generated ones**
  (`CustomizeChromeApiProxy` → `browserProxyFactory` from `*.mojom-webui.js`):
  update overrides and the JS strings inside browser tests, which no build step
  checks. The generated handler includes Brave's mojom additions, so a Brave
  proxy override that only forwarded them can go; upstream's test proxies then
  need stubs for those methods.
- **Elements migrated from Polymer to Lit** (cr159: `site-favicon`): a
  `RegisterPolymerTemplateModifications` override no longer applies; port it to
  a `.html.lit_mangler.ts`. A raw `.html` mangler can't add imports to the
  generated `.html.ts`; once upstream checks in the `.html.ts`, add them with
  plaster. A `chromium_src` override of the element's `.ts` that still imports
  Polymer makes `html_to_wrapper` (which detects the template type from that
  file) emit a Polymer `getTemplate()` wrapper, and `build_ts` fails on
  `getHtml`. Port its styles to `injectStyle` from
  `//resources/brave/lit_overriding.js`, and keep importing the elements the
  template uses, or `lint_ts` reports `web-component-missing-deps`. A
  `RegisterPolymerComponentReplacement` subclass no longer applies either
  (cr159: `site-list-entry`): add the methods the mangled template binds to the
  upstream class's prototype from the `br/` module, declared on its type with
  `declare module '<upstream>.js'` so `getHtml` type-checks, and drop the
  element's `RegisterPolymerComponentToIgnore`, a no-op for Lit.
- **Templates restructured into nested `${...}` templates** (cr159: the settings
  menu's Search item): a mangler can't move elements between templates. Remove
  upstream's element where it is (`mangleAll` with a predicate on the nested
  template) and insert Brave's own where it wants it.
- **Deleted `chromium_src` TS overrides and lit_manglers leave stale build
  output**: the preprocessed file in `gen/` keeps the override or the mangled
  template (as when moving a mangler aside to check that a test fails without
  it), and `html_to_wrapper`'s template detection reads it. `touch` the upstream
  file (an mtime change only) so the step reruns.
- **New lint rules** (`@webui-eslint/lit-element-expressions`): fix every Brave
  template, including strings that lit_manglers inject into upstream templates.
  Boolean attributes set by a mangler use `''`, not `'true'`.
  `@webui-eslint/no-re-exports` (cr159) rejects named re-exports outside barrel
  files: an override re-exports upstream with `export *` (its own exports still
  win), and Brave code imports from the original module.

### Android bytecode redirects

`changeMethodOwner` in `build/android/bytecode` matches by method name only.
When upstream adds a method of the same name (an instance overload), the
redirect rewrites its calls too, and the `__tracerefs` step fails with "DEX
contains references to non-existent symbols". Check whether upstream now does
what the Brave redirect added; if so remove the redirect, its target class, its
`BytecodeTest` check and its `apk_for_test.flags` keep rule (cr159:
`TabBrowserControlsConstraintsHelper.getObservableConstraints`).

When upstream changes the constructor of a class Brave's bytecode adapters
replace with a subclass (`super(` "cannot be applied to given types" in a
`Brave*.java`), change the subclass's signature to match, and the class pair's
`constructorsMatch` list in `android/javatests/.../BytecodeTest.java`: no build
step checks it (cr159: `BraveLocationBarCoordinator`).

## Report

The report is `.autolift/cr<N>/report.md`; keep that name, as the CI job prints
the file in full once the run ends, whether it succeeded or not. So it should
say how far a run that stopped got. `autolift.py prepare`, `lift` and `publish`
append their own sections to it; append yours as you go (decisions, fixes, what
you couldn't verify), and print it at the end. Include:

- A `## Step <n> done: <title>` heading as each step finishes, written by
  `autolift.py step` (see [Procedure](#procedure)).
- The bump range, and the branch range.
- Each commit made, as `<sha> <subject> (culprit <short hash> | no culprit)`,
  with fixups listed under their target. For a fix picked from a newer branch,
  add `(picked from origin/cr<M> <sha>)`. `publish` adds the links to review
  once pushed (see [step 11](#11-publish)).
- For a channel bump, the commits under each branch, the uplifts picked from
  master, and what step 8 would have added to a tracker.
- Each `--tagged=WIP` commit: the concern and the alternatives.
- Issues found but left alone because the codehealth tracker covers them, and
  any test failures attributed to the environment or to flakiness.
- Presubmit warnings left alone, and why.
- Tooling TODOs filed or extended in `tools/cr/codehealth/M<N>.md`.
- Tool bugs fixed during the run, with their commits.
- What wasn't verified: platforms not built (iOS, Windows, macOS), tests that
  couldn't run (official-build-only code, Android without an emulator), and
  fixes verified only statically.

## Maintaining this document

This file should grow from evidence and shrink when advice stops paying off.
Change it only when a run shows that an instruction is wrong, missing or no
longer needed, and as soon as it does, so a later step doesn't repeat the
mistake. A run that only followed it leaves it alone: what happened in the run
goes in the report, not here.

This file is all a CI run knows. An instruction learned about lifts elsewhere
(in a local session, a review, or an agent's own notes or memory) belongs here,
or it is lost to the next run.

### Inputs

- **Corrections.** Places where following this file led to a wrong fix, a wasted
  round, or a commit that had to be amended.
- **New failure mechanisms.** Breakage whose cause and fix
  [Fix patterns](#fix-patterns) doesn't describe.
- **Repeated manual work.** A check run by hand in more than one round, or in
  two runs. If it belongs in `autolift.py`, add it there; if it belongs in
  Brockit, Plaster, `git cr` or another tool, file it under Tooling TODOs (see
  [Tooling](#tooling)).
- **Misses.** Problems in a range a run already lifted that surfaced later,
  through a later rebase, CI or a bug report. Look in `src/brave` history for
  fixes whose "Chromium changes:" link names an upstream commit from an
  already-lifted range, and add what would have caught each one.

### Changing it

- Add only what a run needed, or what would have caught a miss.
- Be judicious: every line here is read by every run, and context the agent
  doesn't need crowds out what it does. Leave out what an agent works out by
  itself from the error, the code or the culprit: general C++, GN or language
  rules, how to apply a single upstream API change, and steps already covered by
  a rule here. Keep what it got wrong, what is specific to Brave's setup, and
  what no error points at.
- Put a rule under the ground rule or procedure step it belongs to, and a fix
  under the [Fix patterns](#fix-patterns) heading for its mechanism. Write one
  bullet per mechanism: the signal and how to check it.
- State what to do, not how a run came to need it: no account of what went wrong
  in a run, which belongs in the report. Give an example only when it shows how
  to recognise the mechanism (a name, a message, an identifier), tagged with its
  milestone, e.g. `(cr159: …)` (untagged ones date from cr158).
- Keep it range-independent. Lists of files, commits and counts go in the
  report, not here.
- If a bullet for the same mechanism exists, sharpen its check rather than
  adding a near-duplicate or a newer example.
- Remove advice when its mechanism is gone, or when tooling now handles it (a
  Brockit step, a presubmit check, a `autolift.py` subcommand). Leave a one-line
  pointer if the tool's name helps.
- When a check is mechanical and has been done by hand in two runs, add it to
  `autolift.py`, with tests in `autolift_test.py`, and shorten the bullet to
  point at it.
- Prefer replacing text to appending it, so the file stays readable in one pass.

### Committing it

- Commit changes to this file, to the `tools/cr/autolift/` helpers, to the CI
  package in `tools/cr/ci/` and to their `tools/cr/README.md` entries with
  `git commit -m 'Version lift updates'`, separately from the lift's fixes and
  from the tracker, so they can be audited. Plain `git commit`: no culprit, no
  tags, no trailers. The subject is pinned in `tools/cr/rebase.py`, so
  `brockit rebase` squashes repeated updates into one commit.
- Tooling TODOs are tracker content: commit them with
  `git commit -m 'Version CodeHealth tracker'`.

# Autonomous Chromium lift

Instructions for an agent that lifts a `cr<milestone>` branch to the latest
Chromium version for that milestone, without a human to ask. The run:

1. resets the local branch to `origin/cr<milestone>`, or creates it for a new
   milestone;
2. rebases it onto the previous milestone's branch;
3. lifts it with 🚀Brockit to the latest tag for the branch;
4. builds Linux and fixes what breaks;
5. builds Android and fixes what breaks;
6. for a new milestone only, publishes the branch and opens its pull request.

Apart from that last step, it ends with the result committed on the local
branch. Don't push, open PRs or comment on GitHub (beyond what
`brockit lift --with-github` updates): the CI job that launched the run decides
what to do with the branch. Keep a running report (see [Report](#report)) and
finish by printing it.

Commands below run from `src/brave` unless they say otherwise. `..` is `src/`,
the Chromium checkout, where the upstream history lives.

## Inputs

- **Milestone `N`** and the branch, `cr<N>`. The branch may not exist yet: for a
  new milestone (e.g. 159, when
  `brockit show --chromium-version-label @latest-tag` is `159.0.8128.3` and
  there's no `origin/cr159`), the run creates it.
- **Base:** `origin/cr<N-1>` while the previous milestone is still on its own
  branch, and `origin/master` once it has landed there. It is set as the
  branch's upstream (see [step 1](#1-reset-and-rebase)).
- **Bump range:** printed by `brockit lift` as "Changes for this bump"
  (`158.0.8117.1..158.0.8127.2`). Culprits for the lift's own breakage are in
  this range. Brockit doesn't print it for a new milestone's first bump;
  `autolift.py lift` then reports the `package.json` versions of the upstream
  and `HEAD` (cr159: `158.0.8127.2..159.0.8131.1`).
- **Branch range:** "Changes since base version"
  (`157.0.8103.15..158.0.8127.2`). It covers every bump already on the branch;
  use it to verify culprit hashes and to look for earlier fixes.
- **Tracker:** `tools/cr/M<N>_codehealth.md`, if it exists. Read it before
  starting: issues it already tracks are left alone (see
  [Ground rules](#ground-rules)).

## Ground rules

These apply to every step. Several came from mistakes that were caught in
review; the reason is given so that edge cases can be judged.

### Scope

- Act only on Brockit sections marked **(action needed)**. Ignore its other
  warnings (e.g. "Patch being flagged as broken, but with unexpected reason",
  "pnpm run init is failing to reset some paths").
- Before fixing a non-trivial runtime or test failure, check
  `M<N>_codehealth.md`. If the issue is already tracked there, leave it and say
  so in the report (cr158: the settings `pref=` → `pref-key` toggle migration).
- Only run tests relevant to the code a fix touches. Full suites are other CI
  jobs' business.
- Don't build while Brockit has open "action needed" items. The tree is
  half-patched and the build only fails at `gn gen`.

### Decisions without a human

When a fix needs a judgement call that would normally go to a reviewer (several
viable designs, a behaviour change, new third-party deps, dropping a Brave
feature), pick the most conservative option that keeps the build going, commit
it with `--tagged=WIP`, and explain the concern and the alternatives in the
commit body and in the report. Then carry on. Examples from cr158:

- Upstream dropped jars Brave's Android icon rasterizer needs from its CIPD
  bundle. Pulling the per-library CIPD packages from `brave/DEPS` keeps the
  rasterizer working without vendoring binaries. That is a new dependency
  source, so it deserves a reviewer's look.
- Migrating 17 settings toggles off a removed binding had three viable designs.
  It was already tracked in the codehealth file, so it was left alone.

### Commits

- Commit with `git cr commit`, never plain `git commit` (except the mangler
  snapshot, below). Pass `--culprit <hash>` whenever there is an upstream
  culprit. The hook writes the "Chromium changes:" block and the `[cr<N>]`
  prefix; don't hand-write either.
- Pass the culprit as a full 40-character hash obtained from
  `git -C .. rev-parse <short>` or copied verbatim from tool output. Never type
  or complete a hash from memory: an invented ending once nearly got committed.
  Confirm that it is in range:
  `git -C .. merge-base --is-ancestor <hash> <new-tag>` succeeds and
  `git -C .. merge-base --is-ancestor <hash> <old-tag>` fails.
- One commit per culprit. If a commit for that culprit already exists on the
  branch, add to it with `git cr commit --fixup=<sha>`, or `--fixup=amend:<sha>`
  when its message must change too (for an `amend!` message, keep the original
  subject and the "Chromium changes:" block, or the squash loses them). Fixups
  are squashed by the next `brockit rebase`.
- Platform-centric fixes get the platform tag: `--tagged=Android` for Android,
  `--tagged=iOS` for iOS (cr158: the iOS key pinning includes). Fixes that
  aren't ready get `--tagged=WIP`. Combine as `--tagged=Android,WIP`; a repeated
  `--tagged` keeps only the last.
- `git cr commit` commits everything staged, and `follow-renames` and `git mv`
  stage as they go. Check `git diff --cached --stat` before each commit (cr159:
  a tool fix swept up the lift's staged renames).
- A fix without an upstream culprit (a latent Brave bug that only now shows up)
  is committed without `--culprit`; say why in the body.
- Simple IWYU fixes, adding an include that a Brave file used to get
  transitively, don't need a culprit hunt. Collect them in one commit,
  `git commit -m 'IWYU fixes.'`, with no body, culprit or tags (the subject is
  pinned, so rebases squash repeats). A fix that needs more than the include (a
  new GN dep, a moved header, a `chromium_src` change) is an ordinary culprit
  fix.
- Messages follow `../agents/shared/skills/cl-description/cl-description.md`: a
  `[Component]`-style subject under 72 columns, counting the `[cr<N>]` and tag
  prefixes the hook adds, and a body wrapped at 72 that explains why. To fix a
  committed subject, add an `amend!` commit with
  `git commit --allow-empty -F <file>`: `amend! <old subject>`, a blank line,
  then the whole new message, prefixes included. Wrap with
  `vpython3 ../agents/shared/skills/cl-description/scripts/wrap_lines.py <file>`
  and pass the file with `-F`. Leave out the `Bug:` line. Never add
  `Co-Authored-By` or any other trailer: on a fixup, a body becomes the squashed
  message, and the hook appends after it.
- In commit subjects and bodies, wrap code names in backticks: classes,
  functions, methods, members, features, files and paths, GN targets, flags and
  commands (e.g. "Pin the `kTtc` default" and "Follow the `TabContainerImpl`
  removal", not "Pin the kTtc default").
- Don't claim in a message what wasn't verified (cr158: "at the version upstream
  last shipped", when only the latest tag had been checked).
- Codehealth tracker changes, including Tooling TODOs, are committed together
  with any `tools/cr/codehealth_review.md` change, with the message
  `Version CodeHealth tracker`.
- Changes to this file and the other `tools/cr/autolift/` files are committed
  with `Version lift updates` (see [Committing it](#committing-it)).

### Editing upstream code

- Make every file change with the Edit and Write tools, one edit per change,
  never with a script (`python3 -`, `sed -i`, `perl -pi`, a heredoc or `>` into
  a source file). That includes resolving conflict markers and editing patches,
  yamls and upstream files. CI approves the Edit and Write tools for anything in
  the checkout, so their edits skip the permission review. A command that
  rewrites files goes through that review instead, which can't see that a change
  only carries a Brave customisation onto new upstream code, and blocks it
  (cr159: a script re-adding Brave's `and False` to `create_string_rc.py` during
  a lift conflict was denied as "Security Weaken"). Tools that write files
  themselves (`plaster apply`, `brockit`, `git`, the `pnpm run` commands) are
  fine. If the review still denies an action, don't try another way to get the
  same result: stop, and record it in the report.
- Upstream files (anything in `src/` outside `src/brave`) change only through
  plaster or a patch. Never run a formatter over them.
- Change a plastered file by editing its `rewrite/**.yaml` and running
  `vpython3 tools/cr/plaster.py apply <yaml>` (or `plaster apply <yaml>`).
  Always pass the yaml; a bare `plaster apply` rewrites every patch. Don't run
  `plaster check` (it is a dry run of `apply`); check the regenerated `.patch`
  and `git diff` instead.
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
  occurrence. See `docs/plaster.md` and `docs/plaster_dos_and_donts.md`.
- When a fix touches a `chromium_src` file that uses a `#define` to rewrite the
  upstream file it includes, consider moving that `#define` to plaster:

  - If the move is simple (a token rename or a hook at one site), do it as part
    of the fix.
  - If keeping it separate makes the fix easier to review, make the migration a
    follow-up commit right after the fix.
  - Commit a migration that stands on its own with
    `git cr commit --tagged=plaster` (plus the culprit, if the migration is what
    fixes it).

  (cr158: the `TabContainerImpl` → `BraveTabContainer` `#define` in the
  `tab_strip.cc` shadow and the `ShowIfNotOffTheRecordProfile` `#define` in the
  `infobar_utils.cc` shadow both became plaster substitutions at their call
  sites.)

- Converting a raw patch to plaster covers the whole `.cc`/`.h` pair: both
  patches, and the `#define`s in both `chromium_src` shadows, so that nothing of
  the pair is left on macros. Go by `docs/plaster.md` and
  `docs/plaster_dos_and_donts.md` (not `.claude/skills/plaster-from-patch`,
  which is outdated):

  - Before replacing a token-rewrite `#define`, list every occurrence of the
    token in the pristine upstream file (`git -C .. show HEAD:<path>`). The
    `#define` rewrote all of them, comments included: cover each one that
    matters (a `make_unique<>` site, a declaration whose type changes) and
    confirm the rest don't (an inherited member pointer or type alias).
  - Anchor an insertion on the most specific line next to it, often a unique
    comment right below the insertion point (`grep -c` it in the pristine file).
    A distinctive line can be a literal `pattern` whose `replace` re-emits it
    after the inserted block (cr158: the web panel block in
    `BrowserView::OnActiveTabChanged`). A generic line, unique only by chance (a
    `DUMP_WILL_BE_NOTREACHED()`, a bare `return`), must be constrained to its
    scope: a `DOTALL` `re_pattern` from the function's definition that can't
    cross its closing brace (`(?:(?!\n\}\n).)*?`), up to the enclosing block
    (cr159: `VectorIconToResourceName` in `searchbox_handler.cc.yaml`).
  - Keep substitutions generic when every occurrence is an intended replacement.
    Swapping in a Brave subclass is a literal `pattern`/`replace` pair,
    `make_unique<Foo>` → `make_unique<BraveFoo>`, with `count: 0` and a comment
    saying every creation wants the subclass. Use `pattern` rather than
    `re_pattern` whenever no regex is needed.
  - `#define Foo BraveFoo` type swaps become `rename_class` when every code
    token of the class in the file should change (cr158: `BookmarkBarView` in
    `browser_view.{h,cc}`); `make_unique<Foo>` swaps alone are the literal pairs
    below.
  - Use the line and import rewriters rather than a regex that re-emits its
    anchor: `add_before_line`/`add_after_line` to insert next to a line, and
    `ts.add_import` for a TypeScript import (cr159: the `security_page.html.ts`
    `loadTimeData` import). `plaster --help` lists them all.
  - Keep a plaster's C++ to a call: anything beyond a trivial substitution goes
    in a helper in the `chromium_src` shadow (an anonymous-namespace function
    before the upstream include), and the `code`/`replace` calls it, passing
    members by reference (`docs/plaster_dos_and_donts.md`; cr159:
    `ShouldOfferBraveShareIcon()` for the sharing hub).
  - `#define X virtual X` becomes `make_virtual`; a `_ChromiumImpl` rename
    becomes `preempt_function_impl` (or a regex insertion at the top of the
    body) with any helper in the `chromium_src` shadow; injected `friend` lines
    become `add_friend`; injected declarations
    `add_to_protected`/`add_to_public`.
  - Look for other shadows that existed only to support the old macros, and
    delete them (cr158: `exclusive_access_context.h` added a
    `UpdateExclusiveAccessBubble_ChromiumImpl` that only `browser_view.cc`
    used).
  - Keep shadow lines that aren't rewrites, such as the Windows
    `#undef LoadAccelerators` that removes `<winuser.h>`'s macro.
  - Check the generated patch reads like hand-written code: inserted blocks
    wrapped, and blank lines kept. Prefer a rewriter (`preempt_function_impl`,
    `make_virtual`, …) over a regex whenever it does the job, even if its output
    isn't indented quite right; file the rewriter's flaw as a Tooling TODO
    rather than working around it with a regex.
  - Build, and list the platforms the build didn't cover in the report.
  - Commit with `git cr commit --tagged=plaster`.

- Plaster `description`s read like a commit message: a subject saying what, a
  blank line, then why. They describe the code as it is for the next reader, not
  how it got there: an upstream flag that was removed or a hook that moved
  belongs in the commit message (cr159: `screenshot_flow.cc.yaml`).
- Keep `set_feature_flag_default_state` / `BASE_OVERRIDDEN_FEATURE` pins even
  when upstream's default now matches. Remove one only if upstream deleted the
  feature, together with its entry in `app/feature_defaults_unittest.cc`.

### Writing Brave code

- Before writing a fix, read the `docs/best_practices.md` topics it touches
  (`chromium-src-overrides.md` for shadows, `build-system.md` for GN,
  `frontend.md` for WebUI, the `coding-standards*.md` for C++, `android.md`),
  and check the result against them before committing.
- `../agents/knowledge_base/rules/` holds Chromium's review rules, each a short
  rule, why, and examples, and they are worth reading for every fix. Each
  directory's `COMMON_METADATA` maps rules to upstream paths (`path_expression`)
  and content (`content_patterns`). It is upstream's, and its paths never name
  `brave/`, so no Brave file matches by itself: map the Brave path to the
  upstream one it stands for before matching (`chromium_src/<path>` and
  `rewrite/<path>.yaml` to `<path>`, `patches/` to the file the patch applies
  to, and Brave code under `brave/browser/`, `brave/components/` or
  `brave/android/` to its upstream counterpart, e.g. `chrome/browser/`). Match
  `content_patterns` against the Brave code itself. Add `general/` for the
  language (`cpp_*`, `java`, `rust`). Treat `severity: should_fix` rules as
  review findings.
- If upstream code builds in Chromium but not in Brave, find the Brave
  discrepancy instead of plastering upstream. The usual leak is
  `-iquote //brave/chromium_src`: every quoted include of an upstream header
  resolves to Brave's shadow, so a Brave header can reach upstream translation
  units several includes deep. For name-lookup errors, look for a Brave nested
  namespace that shadows a global one. The build environment is another source
  (cr158: `PYTHONPATH`, below). Confirm a suspected mechanism with a small
  standalone repro compiled with
  `../third_party/llvm-build/Release+Asserts/bin/clang++ -fsyntax-only` rather
  than a build round. Once the discrepancy is removed, delete any plaster that
  only worked around it: the yaml, its `.patch`, and the upstream file's changes
  (`git -C .. checkout <path>`).
- `auto` only where the Google style guide allows it: the type is obvious
  (`make_unique`, casts, iterators) or doesn't matter.
- Don't add null checks for values that can't be null at the call site, even if
  existing Brave or upstream code does. Use `CHECK()` for a real invariant.
- GN deps go under the same guard as the sources that need them. Don't add a
  platform guard around a dep the target uses unconditionally.
- Run `pnpm run format --base=HEAD` after editing Brave files. It formats only
  the changed lines of Brave files. Revert anything it reformats outside the
  change, such as unrelated tracker files.

### Long commands

- Run commands that take minutes in the background (the Bash tool's
  `run_in_background`): `autolift.py lift` and `lift --continue`,
  `pnpm run init`, the builds, the presubmit and long test runs. Then end the
  turn: you are notified when the command finishes. Don't poll it, `sleep`, or
  read its output in the meantime: every check is another paid turn. Give builds
  the longest background `timeout` (7200000 ms): the 10-minute default kills a
  cold build midway (cr159: after the out dir was wiped). After a kill, "failed
  to flush ... interrupt by signal" in `siso.INFO` is only the kill; check for
  "read 0 bytes" (see [Machine state](#machine-state)).
- Keep their output out of your context: send it to a log in `.autolift/cr<N>/`
  (`> .autolift/cr<N>/<name>.log 2>&1`), as the commands below do. Never let it
  print, and never pipe it through `tee`, `tail`, `head` or `grep` (cr159:
  `autolift.py lift 2>&1 | tail -150` showed nothing for the whole lift). A CI
  run streams every `*.log` there to its console as it is written.
  `autolift.py lift` writes Brockit's output to its own log there, and prints
  only its summary.
- Once it finishes, read only what you need from the log:
  `autolift.py build-summary`, `grep`, `tail -n`. Never the whole log.

### Machine state

- Never delete caches, out directories or anything outside the repo working tree
  (`~/.cache`, the siso cache, `out/`). If one looks corrupt, record it in the
  report and stop. A build killed mid-run can leave truncated blobs in siso's
  local cache, which it then serves back as empty outputs: "failed to parse
  depfile ... EOF" (cr159: `native_browser_test_java__header.d` after a resumed
  run's Android build). Check with
  `grep -c "failed to flush" ../out/<dir>/siso.INFO`; "read 0 bytes: unexpected
  EOF" there means the cache itself is corrupt, so removing the one depfile only
  moves the failure to the next action (cr159: 207 files). One retry tells it
  apart from a transient failure; after that, stop and ask for the cache to be
  cleared.
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
`tools/cr/M<N>_codehealth.md` (create the section if it's missing). Write one
`###` item per tool problem: what the tool does now, the cr<N> example that
showed it, and checkboxes for the changes requested. Extend an existing item
rather than duplicating it. These requests are reviewed separately, so don't
implement missing features or improvements during a lift, and don't let one
block the run. Commit them with `git commit -m 'Version CodeHealth tracker'`,
like other tracker changes.

A hard bug is different: a tool that crashes, corrupts the tree or commits, or
produces a wrong result the lift can't work around. Fix it in the tool, with a
test in its `tools/cr/*_test.py` and the tool's tests passing. Commit the fix on
its own with `git cr commit --tagged=<tool>` (e.g. `--tagged=brockit` for
`brockit.py` or `rebase.py`), never mixed into a lift fix, and list it in the
report. If the fix isn't small and safe, work around the bug, file it as a
Tooling TODO, and carry on.

`tools/cr/autolift/autolift.py` collects the checks done repeatedly by hand:

```sh
# Run or continue brockit lift, and summarise what needs attention (step 2).
vpython3 tools/cr/autolift/autolift.py lift
vpython3 tools/cr/autolift/autolift.py lift --continue

# Reset cr<N>, set its base as upstream, and start brockit rebase (step 1).
vpython3 tools/cr/autolift/autolift.py prepare 158

# New milestone only, at the very end: push it and open its PR (step 11).
vpython3 tools/cr/autolift/autolift.py publish 159

# Outcome, failed steps and distinct errors of a build log.
vpython3 tools/cr/autolift/autolift.py build-summary .autolift/cr158/build-linux.log

# Upstream commits in the bump range touching paths, or adding/removing a string.
vpython3 tools/cr/autolift/autolift.py culprits 158.0.8117.1..158.0.8127.2 net/http/foo.cc
vpython3 tools/cr/autolift/autolift.py culprits 158.0.8117.1..158.0.8127.2 --symbol kFooFeature

# Check every culprit cited since the base: it resolves, and it's in range.
vpython3 tools/cr/autolift/autolift.py verify-culprits origin/cr157 157.0.8103.15..158.0.8127.2
```

The run keeps its logs and its report in `.autolift/cr<N>/` in `src/brave`
(git-ignored): inside the workspace, and outside `out/`, so that a clean rebuild
(`rm -fr ../out`) doesn't wipe them. Don't keep them in `/tmp`. `prepare` and
`lift` write there themselves; send build logs there too. `grep` may be aliased
to `ugrep`, which skips binary files and handles `--include` differently; use
Python for binary inspection.

## Procedure

Once a step is done, append `## Step <n> done: <title>` to the report, with its
outcome in a line or two (for step 3: "`brave:all` builds; 4 fixes"). These
headings are what a resumed run goes by.

### Resuming a run

A run that stopped before the end (an error, an API outage, the job being
stopped) can be resumed: the CI job then keeps the checkout, the out dir and
`.autolift/cr<N>/` as the run left them, and asks you to resume instead of
starting over.

- Don't run `autolift.py prepare` again: it resets `cr<N>` to `origin/cr<N>`,
  which drops the stopped run's commits.
- Carry on with the step after the last `## Step <n> done` heading in the
  report. With no such heading, work out where the run got to from the report's
  other sections, the logs in `.autolift/cr<N>/`, `git log <base>..HEAD` and
  `git status`.
- Check what the interrupted step left first: a rebase or `brockit lift` that
  stopped midway, conflict markers, uncommitted changes, a build or test log
  that ends abruptly. Finish that step, or redo it, before going on.
- Compare `git reflog` with the report's commit lists: the branch can be
  rewritten outside the run between sessions (cr159: teammates' fixes
  cherry-picked on top, and two of the run's fixups dropped). Don't undo it;
  list it in the report, and say which steps it came after and so wasn't
  verified by.
- Subagents don't survive an interruption. If step 8 stopped with sweeps
  running, rerun them all on the existing dump directory (complete if `subjects`
  has the range's commit count), ignoring the stopped sweeps' scratch files.
- Append `## Resumed (<date>)` to the report, saying which step you picked up at
  and why.

### 1. Reset and rebase

```sh
vpython3 tools/cr/autolift/autolift.py prepare <N>
```

It fetches `master` and `cr<N>`, picks the base, checks out `cr<N>` reset to
`origin/cr<N>`, sets the base as its upstream, and starts
`brockit rebase --squash-minor-bumps --discard-regen-changes`. It prints the
base it picked: use it wherever this document says `<base>`, such as
`verify-culprits`. It refuses to run during a rebase, when tracked files have
changes, or when the local `cr<N>` has commits `origin/cr<N>` lacks, since the
reset would discard them. In CI, where the checkout is fresh, none of these
should happen; if one does, stop and report it rather than passing `--force`.

For a new milestone, with no `origin/cr<N>` yet, it creates `cr<N>` from the
base (for 159: from `origin/cr158`), sets the same upstream, and skips the
rebase, since the branch has nothing of its own yet. Go straight to
[step 2](#2-lift): `--to=@latest-for-branch` takes the major from the branch
name. The new branch stays local until [step 11](#11-publish-a-new-milestone).

The base is `origin/master` once master's Chromium version is on milestone
`N-1`, which is when the previous milestone has landed, and `origin/cr<N-1>`
before that. Brockit takes the branch's upstream as the base for both
`brockit rebase` and `brockit lift` (`--from-ref` defaults to `@upstream`).

**When the previous milestone lands.** The first run after `cr<N-1>` merges into
master moves `cr<N>` onto master (cr158 on 2026-10-29, when cr157 landed as
"Upgrade from Chromium 156 to Chromium 157" and `origin/cr157` was deleted).
`prepare` picks `origin/master` by itself, and says so in its output and the
report ("Base moves to origin/master"), because `origin/cr<N>` still forks from
master as it was before the merge. In that rebase:

- Only the branch's own commits are replayed: Brockit starts after the previous
  milestone's commits, which master now has. Expect more conflicts than usual:
  master's own changes from the previous milestone's cycle arrive all at once.
  Resolve them with the recipes below, against master's version of each file.
- `origin/cr<N-1>` may no longer exist. Use `origin/master` wherever this
  document says `<base>`, including `verify-culprits`.
- The branch range ("Changes since base version") now starts at master's
  Chromium version.
- The `cr<N>` pull request, if there is one, still targets `cr<N-1>`. Don't
  retarget it yourself; list it in the report for the CI job.

The rebase also autosquashes `fixup!` and `amend!` commits. Brockit resolves the
conflicts that have a fixed answer as it goes:

- a patch with a yaml is regenerated from its plaster;
- a `tools/cr/M<N>_codehealth.md` the base doesn't have takes the branch's last
  version;
- the "Update from Chromium" commit takes its own `projects.chrome.tag` and the
  newer `PinsListTimestamp`, when that is the file's single conflicting hunk;
- a raw patch the base changed only in its context takes the commit's version.

It stops on any other conflict, listing the commit and the files left. Resolve
them with the recipes below, `git add` them, and carry on with:

```sh
vpython3 tools/cr/brockit.py rebase --continue
```

To drop the commit it stopped on, use
`vpython3 tools/cr/brockit.py rebase --skip`. Never use plain
`git rebase --continue` or `--skip`: they skip Brockit's handling of the squash
messages and its conflict rules for the rest of the rebase. When the rebase
finishes, Brockit lists what it resolved, by rule. Recipes for what it leaves:

- **`patches/*.patch` with a yaml:** Brockit leaves these when the yaml
  conflicts too, when the plaster no longer applies, or when the Chromium
  checkout isn't on the version rebased so far (it warns). Don't resolve
  anything in the patch. Resolve a conflicted yaml first: usually the newer
  design on the base branch supersedes the branch's version (cr158: the `.onion`
  CORS fix in cr157 covered the preflight case the cr158 commit added). Check
  that it still applies to the new source and covers what the branch's version
  did, then regenerate the patch with `plaster apply patches/<name>.patch`. The
  same patch may conflict again in a later "Update patches" commit; regenerate
  it again.
- **`tools/cr/M<N>_codehealth.md`**, when the base has a copy too: take
  `origin/cr<N>`'s version.
- **A cr<N> fix that the base fixed itself** (cr159: guards for cr158 commits
  that were only built on Linux): take the base's version. A commit left empty
  is dropped with `brockit.py rebase --skip`; one left partly applied gets an
  `amend!` that rewords it to what remains.
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
  adds its yaml (cr158: `location_bar_view.cc`, converted earlier in the same
  branch): after the rebase, check whether the branch tip has a yaml for each
  one. Convert each one that is still raw in a follow-up commit, as below.
- **A `fixup!` of a pinned group that touches files the group moves later**
  (modify/delete conflict): Brockit places such fixups right after the group's
  first commit. Keep HEAD's state for that step (and undo anything git's rename
  detection applied to the old path), then requeue the same `fixup <sha>` line
  after the group's last `squash` line in
  `$(git rev-parse --git-path rebase-merge)/git-rebase-todo`, before running
  `--continue`. Check afterwards that the squashed commit has the fixup's
  change.
- **Lit mangler snapshot** (`tools/chromium_src/lit_mangler/__snapshots__/`):
  resolve and add. It is regenerated at the end.
- **Test filters and other files:** resolve on the merits. When both sides
  filter the same test, keep the broader filter, whichever side it is on.

A commit whose changes are already on the base branch is dropped automatically
("patch contents already upstream"). File a Tooling TODO for any recipe you had
to apply by hand that Brockit could have applied.

### 2. Lift

```sh
vpython3 tools/cr/autolift/autolift.py lift
```

It runs `brockit lift --infra-mode --with-github --to=@latest-for-branch`
(always with `--with-github`), keeps Brockit's full log in `.autolift/cr<N>/`,
and prints (and appends to the report) the bump and branch ranges and what needs
attention: each "action needed" section with its items, failed checks (❌) and
errors (🔥). Read the log only for detail. Brockit updates Chromium, reapplies
patches with `--3way`, and stops with "action needed" sections. Handle each
section, then run:

```sh
vpython3 tools/cr/autolift/autolift.py lift --continue
```

`lift` exits non-zero whenever anything needs attention, including a failed
plaster check after `--continue`, which Brockit itself exits 0 on. A zero exit
means the lift is done.

On a new major version (cr159's first lift, from `158.0.8127.2` to
`159.0.8130.0`), Brockit first triggers the CI pipelines that build the Rust
toolchain for the new tag, and watches them ("Triggering pipelines for Rust
toolchain"). No commit appears until they finish, which takes a long time: let
it run. This needs CI (Jenkins) credentials on the machine (`~/.jenkins.json`).
If a pipeline fails, the lift stops there with no commits, and there is nothing
to `--continue`. Stop the run: don't retry the pipeline or work around the
toolchain. Report which pipeline failed and its link, and leave the branch as
`prepare` left it. A later run starts again from step 1.

For each item, find the culprit first
(`autolift.py culprits <bump range> <path>`), then fix:

- **Files that cannot be patched anymore: renamed or moved.** Run
  `git cr follow-renames <full culprit hash>`. It moves patches and yamls and
  updates C++ includes and GN labels, but not Java imports, nor namespaces or GN
  target labels that changed with the move. Grep for the old package, path and
  symbol names in Java, C++, GN, DEPS, `test/filters` and
  `app/feature_defaults_unittest.cc`, and fix what remains. Its formatter also
  reflows unrelated tracker files; revert those. Check that the patch applied to
  the moved upstream file. cr159: settings checked in its Lit templates, so
  `foo.html` became `foo.html.ts`; the `.lit_mangler.ts` files move with them,
  and a patch's `<if expr>` wrappers must stay inside `getHtml()`'s template
  literal (a closing `</if>` landed after the function).
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
    `ENABLE_DESKTOP_ANDROID_EXTENSIONS` became `IS_ANDROID`);
  - the feature is gone: drop that substitution and its
    `feature_defaults_unittest.cc` entry;
  - the code changed shape: rewrite the substitution.
- **Manually resolve conflicts for.** Open the upstream file in `src/`.
  - _Minor_ (context drift, the Brave hook still has a home): resolve the
    markers and stop. `--continue` refreshes the patch, so don't regenerate or
    commit it.
  - _Non-trivial_ (the code the hook targeted is gone or moved): resolve it,
    rebuild the customisation on the new code, regenerate or delete the patch
    with `pnpm run update_patches <path>`, and commit with the culprit. cr158:
    static key pinning moved from `TransportSecurityState` into
    `KeyPinningStateChrome`. The constructor hook was replaced by a plaster on
    `kDefaultEnableStaticPins`, and the same culprit turned key pinning off on
    iOS, which Brave needs, so `chrome_key_pinning_supported = true` went into
    `build/args/ios_defaults.gni`. Read the whole culprit, not just the
    conflicting hunk.

After `--continue`:

- "❌ Plaster check. Please investigate it." Run `plaster apply --all` (the one
  sanctioned unscoped use) and inspect the diff. Nothing changed: carry on
  (stale `.patchinfo` metadata is a likely cause). Unremarkable, sensible
  changes: commit as `--fixup` to the "Apply-fixed 🩹 patches from ..." commit.
  Significant changes: their own commit with a culprit, `--tagged=WIP` if
  unsure. A yaml that fails ("Unexpected number of matches") is handled like
  "Plaster failed to fix patches" above (cr159: upstream moved the searchbox
  icon lookup into another function; the old patch applied there with fuzz, but
  the regex no longer matched).

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

1. `autolift.py build-summary .autolift/cr<N>/build-linux.log`.
2. Steps running in parallel may fail together. Group their errors by root
   cause, not by file: one header change can fail many objects.
3. For each group, find the culprit and the fix (see
   [Fix patterns](#fix-patterns)). Independent groups can be researched in
   parallel, by subagents that only research. Edits and commits stay in the main
   agent so they don't race.
4. Commit each fix with its culprit, re-run `pnpm run format --base=HEAD` on
   Brave files, and rebuild.

If a log ends with an error about `out/Component/build.guard`, two builds
overlapped. Make sure only one runs.

To check one fix quickly, build just the targets it touches with the same
command and flags, e.g. `--target=brave_unit_tests`,
`--target=chrome/browser/resources/settings:lint_ts` or
`--target=chrome/test/data/webui/tab_search:build_ts`. A full `brave:all` build
is still what closes the step.

### 4. Linux tests

Once the build is green, run the tests that cover each fix, and add coverage
where a runtime fix has none.

```sh
vpython3 tools/recipes/runtest.py --build-dir ../out/Component --xvfb \
    <binary> --no-sandbox --gtest_filter='<Suite>.*'
```

The binary is a name relative to `--build-dir` (`brave_unit_tests`,
`brave_browser_tests`, `brave_cert_pinning_tests`, upstream `unit_tests`). Check
that the filter matched something: "Tests took 0 seconds" or `[0/0]` means it
didn't (fixture names are case-sensitive: `SpeedReaderBrowserTest`, not
`SpeedreaderBrowserTest`).

Finding coverage:

- Search Brave test files for the class or function a fix touches. Feature
  default pins are covered by `FeatureDefaultsTest.*` in `brave_unit_tests`.
- Brave tests of overridden upstream code live at the matching
  `chromium_src/.../*_unittest.cc` path and are listed in `test/BUILD.gn`'s
  `brave_unit_tests` sources. A `chromium_src` shadow of an upstream unittest
  can include it and reuse its fixtures and helpers.
- A new test must fail without the fix. Assert behaviour, not a value you just
  set. In cr158, the first test for the reader mode toolbar asserted the
  accelerator mode reported for the web contents. It failed, and that exposed
  that the fix itself was wrong (`SetAcceleratorHandling()` only applies when
  the `WebView` is the contents' delegate). The final test checks that a browser
  accelerator isn't held back for the renderer. For a plastered fix, check this
  directly: remove the plaster's lines from the upstream file, rebuild the test
  target and run it, then restore them with `plaster apply <yaml>` and check
  that the `.patch` is unchanged (cr159: `BraveThemeSourceTest` crashed without
  the system-profile check).
- Some code only builds in official builds (e.g. `enable_crash_dialog`). Its
  tests can't run in a Component build: write them, and list them as unverified
  in the report.
- Commit tests as a `--fixup` to their fix's commit.

Known environment-only failures, not lift regressions:

- `BraveFontWhitelistTest.*` fails on Linux distros that `GetLinuxDistro()`
  doesn't recognise (e.g. Ubuntu 26.04).
- Tests that pass on retry are flaky; report them, don't fix.
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
manglers.

### 6. Android

```sh
pnpm run init --target_os=android > .autolift/cr<N>/init-android.log 2>&1
pnpm run build Static --target_os=android --target_arch=x64 \
    --target=brave:all --gn=android_static_analysis:on \
    > .autolift/cr<N>/build-android.log 2>&1
```

Use `x64`, so that tests can run on an emulator. `android_static_analysis=on`
runs Android lint, error-prone and the bytecode checks as blocking build steps,
so their findings fail this build; by default (`build_server`) they run in a
daemon and don't block. The out dir is `../out/android_Static`. The build starts
with `gn gen --check`, which catches deps that a Linux-only `gn check` missed
(cr159: a cr158 change to direct deps left three Brave targets without their
Android deps); when a lift changes GN deps, expect Android to need its own. Read
the `gn gen` warnings too: "Build argument has no effect" on an arg Brave sets
means upstream removed it, and the build stays green without it (cr159:
`android_override_version_name`, so the APK lost Brave's version name). Like the
Linux build, it stops at the first failure. Re-run
`pnpm run init --target_os=android` after changing `brave/DEPS`. It also checks
that the new deps sync. All Android fixes get `--tagged=Android`.

For tests, run only the Android tests for code touched by Android fixes.
Robolectric tests run on the host, with `brave/script` on `PYTHONPATH`:

```sh
PYTHONPATH=$PWD/script ../out/android_Static/bin/run_brave_junit_tests \
    -f '*<Suite>*' > .autolift/cr<N>/test-junit.log 2>&1
```

The whole suite takes under a minute (cr159: 1179 tests). Instrumentation tests,
such as `BytecodeTest`, need an emulator: `tools/android/avd/avd.py install` and
`start` with an x64 config such as
`tools/android/avd/proto/android_34_google_apis_x64.textpb`. That downloads
several GB and needs `/dev/kvm`. If no device can be had, list the Android tests
as unverified in the report.

### 7. Presubmit

Once the Linux and Android builds pass, run the presubmit against the branch's
upstream (the base):

```sh
pnpm run presubmit --base "$(git rev-parse --abbrev-ref '@{upstream}')" \
    > .autolift/cr<N>/presubmit.log 2>&1
```

Address what it reports, errors above all, rebuild the targets a fix touches,
and run the presubmit again until no errors are left. Address warnings too when
they point at a real problem. Some warnings aren't worth addressing, or are
outdated: the one about empty lines in patches no longer matters, as plaster
generates those patches. List each warning left alone, and why, in the report.

A finding is almost always in code that a commit on the branch added, so fix it
in that commit, as a fixup. Find the commit that last touched the flagged line:

```sh
git log --no-patch --format='%H %s' -1 -L<line>,+1:<file> <base>..HEAD
```

and commit the fix with `git cr commit --fixup=<sha>`, using the commit a
`fixup!` fixes when the one found is itself a fixup (cr159: pylint findings in
the run's own tooling changes were committed as separate fixes instead). A fix
is a commit of its own only when no branch commit introduced the problem: the
line is unchanged from the base, or upstream changed what makes it wrong, in
which case it gets that culprit.

### 8. Code health review

Once the presubmit is addressed, run `tools/cr/codehealth_review.md` over the
run's whole range, `<old tag>..<new tag>` (cr159: `158.0.8127.2..159.0.8131.1`).
It writes or updates `tools/cr/M<N>_codehealth.md` with what fails silently:
overrides that stopped applying, re-enabled features, Brave copies that missed
upstream fixes. For a branch that isn't on origin yet, "what the rebase already
fixed" is `<base>..cr<N>` locally, not `origin/master..origin/cr<N>`. Follow its
own instructions (subagents, static verification only), and commit the tracker
with any change to `codehealth_review.md` as `Version CodeHealth tracker`.

The review takes a long time: its sweeps run for an hour or more each, in
parallel, and that is expected. Start them, end your turn, and wait for their
notifications, however long they take. Never stop a sweep, or write the tracker
without one, to finish sooner (cr159: the run ended with every sweep still
running, and the tracker had nothing for the range).

The review is finished only when every subagent it started has come back with
its final result. A completion notice that says the agent still has background
work running is interim: wait for its final notice, and check that no review
agent is still listed as running, before writing the tracker. Then fold in
anything that arrived after the tracker was written before publishing (cr159 was
published before one sweep had finished). Tell each review subagent not to leave
commands running in the background once it has its result, and before writing
the tracker check `pgrep -af 'git grep|rg |python'` for strays left by a sweep
(cr159: two slow `git grep -f` runs outlived the sweep that started them, and
kept it from finishing).

Don't fix what the review finds, regressions included: the tracker is where
they're picked up, so list them in the report and carry on. It is the last step
before the run stops, or before a new milestone is published: don't publish
without it.

### 9. Update this document

Before finishing, fold what the run taught into this file, following
[Maintaining this document](#maintaining-this-document), and commit it.

### 10. Wrap up

- `autolift.py verify-culprits <base> <branch range>` reports no problems.
- `git status` is clean. Untracked files are scratch you created, or deps
  fetched into git-ignored paths.
- No build is left running.
- Print the report.

### 11. Publish a new milestone

Only when `prepare` created the branch in step 1, because `origin/cr<N>` didn't
exist, and only once the whole run is done: the lift finished, the Linux and
Android builds pass, the presubmit reports no errors, the code health review is
complete (every review subagent back) and committed, and wrap-up is clean. Then:

```sh
vpython3 tools/cr/autolift/autolift.py publish <N>
```

It pushes `cr<N>` to origin without changing its upstream, which stays the base,
and runs `brockit update-version-issue`. That creates the version issue and a
pull request against the base. It refuses to push a branch that already exists
on origin, or one with no commits on top of its base. A run on an existing
branch never pushes. If the builds aren't green, don't publish: report why, and
leave the branch local.

## Fix patterns

Each of these came up in the cr158 lift. Find the culprit first: the pattern
tells you where to look, the culprit tells you what upstream intended.

### Moves and renames

- **Java package moves** (`RecentlyClosedEntriesManager` to `recent_tabs`):
  `follow-renames` doesn't update Java imports. Fix them by hand in `android/`,
  including `junit` tests, then run `pnpm run format --base=HEAD`.
- **Namespace moves** (`kBrowserActuatorProtoStreamTransport` to
  `browser_actuator::transport`): update the qualified name, the include, the GN
  dep and the `DEPS` include rule.
- **Enum renames** (`WindowOpenDisposition::CURRENT_TAB` → `kCurrentTab`, no
  aliases): one mechanical rename across Brave in a single commit. Map every
  value, update comments that name the old values, then format. Check Java and
  TS separately (mojom Java bindings kept the old names).
- **Deleted third-party targets** (`//third_party/kotlin_stdlib`): use the label
  upstream's own build files now use
  (`//third_party/android_deps:kotlin_stdlib_java`).

### Signature and API changes

- **Brave overrides of a changed signature.** When a Brave `#define` declares an
  `_Unused` duplicate of an upstream method (`UpdateKeyPinsList`), update both
  declarations. If the new signature has a reply callback, run it, or callers
  waiting on the reply hang.
- **Removed setters.** Read how upstream migrated its own callers, and check
  that the replacement applies in Brave's situation. `SetAcceleratorHandling()`
  replaced `set_allow_accelerators(true)` only for a `WebView` that is its
  contents' delegate; Brave's reader mode toolbar isn't, so it needed
  `set_allow_accelerators_deprecated(true)`, as upstream UI hosts in the same
  position use. A test caught this.
- **Classes replaced by free functions** (`SessionCrashedBubble` →
  `ShowSessionCrashedBubbleIfNotOffTheRecordProfile()`): a Brave `#define` on
  the old method name compiles nothing and silently drops Brave's hook (here,
  the crash report permission dialog). Re-route with a plaster at the call site,
  and declare the Brave wrapper in the `chromium_src` header.
- **Class moved out of a namespace** (`ChromeSyncablePrefsDatabase` out of
  `browser_sync`): move Brave's definitions out too. Check that everything
  inside was fully qualified.
- **Brave appends to an upstream enum after `kMaxValue`** (`DownloadCommands`):
  when upstream adds a value, renumber Brave's values past it and update the
  `static_assert`. Check first that Brave's values aren't persisted or recorded
  in histograms.
- **Brave stubs out a whole upstream `.cc`** (`prerender_manager.cc`): new
  `extern` declarations in the header need definitions in the stub, or tests
  fail to link.

### Lost transitive includes

Upstream forward-declares something (`base::OneShotTimer`, `GURL`) and a Brave
file that relied on the old include chain breaks. Add the include to the Brave
file; don't plaster upstream. When adding the include is the whole fix, commit
it with the run's other such fixes as `git commit -m 'IWYU fixes.'` (see
[Commits](#commits)), without naming a culprit. When the fix needs more, or the
failure looks like more than a lost include, name the culprit:

1. Preprocess the failing translation unit to list what it includes now. Take
   its compile command from the log (the `clang++ ... -c x.cc -o x.o` line),
   drop `-MMD -MF <file>`, and replace `-c <src> -o <obj>` with
   `-E -H <src> -o /dev/null`. Run it from the out dir: `-H` writes the include
   tree to stderr. This is a preprocessor run, not a build.
2. List the headers the range removed the include from:
   `git -C .. log <bump range> -G'#include "url/gurl.h"' -- '*.h'`. For each
   commit, see which headers lost the include.
3. The culprit is the commit whose header is in the include list.

### Build system and environment

- **New upstream deps in a test Brave compiles** (`gn check`: "Can't include
  this header from here" in an upstream test listed in Brave's
  `android/android_browser_tests.gni`): add the same deps upstream added in the
  culprit.
- **GN visibility**: a template now depends on a target with restricted
  visibility (crubit `cpp_api_from_rust` bindings on their crate). Add the
  dependent to the `visibility` list.
- **Brave-only environment leaks**: an upstream action that works in Chromium
  but fails in Brave can be Brave's `pnpm run build` environment. cr158:
  `build/commands/lib/config.ts` put `depot_tools` on `PYTHONPATH`, and its
  `utils.py` shadowed SPIRV-Tools' `utils` package. Confirm by running the
  action's import with and without the variable. Fix at the source: dropping the
  entry and making its last user add the path itself.
- **Upstream drops a library Brave uses** (Android-tools jars in the
  `android_deps` CIPD bundle): Brave's `DEPS` can declare per-library CIPD
  packages (`"dep_type": "cipd"`, `"condition": "checkout_android"`), fetched
  into a git-ignored directory. Look for them with
  `cipd ls chromium/third_party/android_deps/libs` and
  `cipd describe <package> -version latest`. Check the package layout with
  `cipd export` into a scratch directory. A new dependency source is a
  `--tagged=WIP` decision.
- **Stale Brave workarounds**: when an upstream change fixes what a Brave
  workaround worked around, the workaround can become the bug. cr158: a `dex.py`
  plaster kept the WireGuard prebuilt out of intermediate dexing because D8
  rejected its Java record there. Upstream then made intermediate dexing drop
  global synthetics and add them back in the final dex. The plaster then made
  WireGuard ship its own `RecordTag`, which duplicated upstream's. Look for a
  Brave plaster or patch on the failing tool before suspecting upstream.
  Reproduce by replaying the action's exact command (from the
  `rule ... command =` line in the out dir's `.ninja` files), with its outputs
  redirected to a scratch directory.
- **Stale outputs vs. real bugs**: before blaming a cache, compare the output
  with what the action produces when replayed. In cr158, the WireGuard dex was
  correct for the command actually run; the command was wrong.

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
  need stubs for those methods (cr159: tab search).
- **Elements migrated from Polymer to Lit** (cr159: `site-favicon`): a
  `RegisterPolymerTemplateModifications` override no longer applies; port it to
  a `.html.lit_mangler.ts`. A raw `.html` mangler can't add imports to the
  generated `.html.ts`; once upstream checks in the `.html.ts`, add them with
  plaster.
- **Templates restructured into nested `${...}` templates** (cr159: the settings
  menu's Search item): a mangler can't move elements between templates. Remove
  upstream's element where it is (`mangleAll` with a predicate on the nested
  template) and insert Brave's own where it wants it.
- **Deleted `chromium_src` TS overrides leave stale build output**: the
  preprocessed file in `gen/` keeps the override, and `html_to_wrapper`'s
  template detection reads it. `touch` the upstream file (an mtime change only)
  so the step reruns.
- **New lint rules** (`@webui-eslint/lit-element-expressions`): fix every Brave
  template, including strings that lit_manglers inject into upstream templates.
  Boolean attributes set by a mangler use `''`, not `'true'`.
  `@webui-eslint/no-re-exports` (cr159) rejects named re-exports outside barrel
  files: an override re-exports upstream with `export *` (its own exports still
  win), and Brave code imports from the original module.
- **Brave replacements of upstream elements** (Brave's `cr-button`): declare
  properties upstream code now binds (`noink`) even if Brave's element ignores
  them.

### Android bytecode redirects

`changeMethodOwner` in `build/android/bytecode` matches by method name only.
When upstream adds a method of the same name (an instance overload), the
redirect rewrites its calls too, and the `__tracerefs` step fails with "DEX
contains references to non-existent symbols". Check whether upstream now does
what the Brave redirect added; if so remove the redirect, its target class, its
`BytecodeTest` check and its `apk_for_test.flags` keep rule (cr159:
`TabBrowserControlsConstraintsHelper.getObservableConstraints`).

### Android 32-bit

`std::ranges::count_if` returns `ptrdiff_t`, which is `int` on 32-bit ARM, so
comparisons with unsigned literals trip `-Wsign-compare`. Compare with signed
literals. This was a latent Brave bug with no upstream culprit.

### Android static analysis

With `android_static_analysis=on`, error-prone and NullAway warnings fail the
build ("Command failed because it wrote to stderr"), and `build-summary` shows
no distinct errors: read the `warning: [Check]` lines after the failed
`__errorprone` step. Nullable arrays are `byte @Nullable []`; `@Nullable byte[]`
annotates the element type and trips `NullablePrimitiveArray` (cr159, after the
NullAway update in the bump). Type-use positions need
`org.chromium.build.annotations.Nullable`; `androidx.annotation.Nullable` isn't
applicable there. A lambda parameter needs the same nullability as the interface
method it implements.

## Report

The report is `.autolift/cr<N>/report.md`. `autolift.py prepare` and `lift`
append their own sections to it; append yours as you go (decisions, fixes, what
you couldn't verify), and print it at the end. Include:

- A `## Step <n> done: <title>` heading as each step finishes (see
  [Procedure](#procedure)).
- The bump range, and the branch range.
- Each commit made, as `<sha> <subject> (culprit <short hash> | no culprit)`,
  with fixups listed under their target.
- Each `--tagged=WIP` commit: the concern and the alternatives.
- Issues found but left alone because the codehealth tracker covers them, and
  any test failures attributed to the environment or to flakiness.
- Presubmit warnings left alone, and why.
- Tooling TODOs filed or extended in `M<N>_codehealth.md`.
- Tool bugs fixed during the run, with their commits.
- What wasn't verified: platforms not built (iOS, Windows, macOS), tests that
  couldn't run (official-build-only code, Android without an emulator), and
  fixes verified only statically.

## Maintaining this document

This file should grow from evidence and shrink when advice stops paying off.
Update it at the end of every run, and as soon as something in it proves wrong
mid-run: a later step shouldn't repeat the mistake.

This file is all a CI run knows. Anything learned about lifts elsewhere (in a
local session, a review, or an agent's own notes or memory) belongs here, or it
is lost to the next run.

### Inputs

- **Corrections.** Places where following this file led to a wrong fix, a wasted
  round, or a commit that had to be amended (cr158: setting the upstream before
  `brockit rebase` was missing, so a fresh checkout would rebase onto itself).
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
- Put a rule under the ground rule or procedure step it belongs to, and a fix
  under the [Fix patterns](#fix-patterns) heading for its mechanism. Write one
  bullet per mechanism: the signal, how to check it, and one concrete example in
  parentheses.
- Tag new examples with their milestone, e.g. `(cr159: …)`. Untagged examples
  date from cr158.
- Keep it range-independent. Lists of files, commits and counts go in the
  report, not here.
- If a bullet for the same mechanism exists, refresh its example or sharpen its
  check rather than adding a near-duplicate.
- Remove advice when its mechanism is gone, or when tooling now handles it (a
  Brockit step, a presubmit check, a `autolift.py` subcommand). Leave a one-line
  pointer if the tool's name helps.
- When a check is mechanical and has been done by hand in two runs, add it to
  `autolift.py`, with tests in `autolift_test.py`, and shorten the bullet to
  point at it.
- Prefer replacing text to appending it, so the file stays readable in one pass.

### Committing it

- Commit changes to this file, to the `tools/cr/autolift/` helpers and to their
  `tools/cr/README.md` entry with `git commit -m 'Version lift updates'`,
  separately from the lift's fixes and from the tracker, so they can be audited.
  Plain `git commit`: no culprit, no tags, no trailers. The subject is pinned in
  `tools/cr/rebase.py`, so `brockit rebase` squashes repeated updates into one
  commit.
- Tooling TODOs are tracker content: commit them with
  `git commit -m 'Version CodeHealth tracker'`.

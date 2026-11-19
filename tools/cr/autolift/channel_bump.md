# Channel bump run

Read with [SKILL.md](SKILL.md), whose ground rules and Procedure this changes.

Given a Chromium tag (`157.0.8103.38`), the run bumps Brave's release channels
to it: master (nightly), and the beta and release `1.<minor>.x` branches below
master's Brave version, each one that is on the tag's milestone M<N> and behind
it. Each gets a branch off the channel, with the channel as upstream (`cr157.x`
off `origin/master`, `cr157.x-on-1.100.x` off `origin/1.100.x`), and a pull
request against it. The run keeps its report and logs in `.autolift/cr<N>.x/`.
The [Procedure](SKILL.md#procedure)'s steps apply, with these differences:

- **Tooling.** The channels don't carry this tooling. `prepare <tag>` copies
  `tools/cr` to `.autolift/tooling/cr/`, which checking out a channel leaves
  alone: from then on run `vpython3 .autolift/tooling/cr/autolift/autolift.py`,
  and read these files there. `brockit`, `plaster`, `git cr` and `pnpm` are the
  checked-out channel's own. There is no tooling sync. Step 10's changes are
  made in `.autolift/tooling/cr/` and committed as `Version lift updates` on the
  master channel's branch only (into its `tools/cr/`, written whole where master
  lacks the file); `channel-picks` leaves them out. No branch takes a
  `Version CodeHealth tracker` commit: what step 9 would add to a tracker goes
  in the report.
- **Step 1** is `prepare <tag>`. It creates the branches, with no rebase, and
  checks out the highest channel's. The names repeat from one bump of M<N> to
  the next: a branch an earlier bump left on GitHub is pushed over at step 12. A
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
  [step 1](SKILL.md#1-reset-and-rebase), then `channel-picks`: a channel left as
  it is still gives its fixes to those below it. A backport the channel took
  after the bump (cr157: the WebUSB backport, uplifted to beta and release) now
  needs the higher channel's revert of it. `publish` rebases again only if the
  channel moves once more.
- **Steps 2 to 5 and 7** run on the highest channel's branch, and `step` records
  them on the checked-out branch: `status` names the step and the branch to
  carry on with. Name logs after the channel
  (`.autolift/cr<N>.x/build-linux-master.log`). The lower channels take step 2
  alone, one at a time once the highest one's step 9 is done: the lift and
  `channel-picks`, with no build, tests or presubmit of their own, as the pull
  requests' CI and an autofix run cover them, unless there is a good reason to
  (say why in the report). There is no Android build (step 6) either. The
  checkout is synced to one branch at a time: after checking out another for
  anything but its lift (which syncs itself), run `pnpm run sync` before
  building, testing or checking it, or the checks read the other channel's
  patches (cr157: a false plaster error). `publish` and `autofix` sync each
  branch they check out to build or check. The lift is a minor bump: look for
  the newer branches' fixes as in
  [Minor bumps on a maintenance branch](SKILL.md#minor-bumps-on-a-maintenance-branch)
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
- **There is no step 8**: the channels' pull requests have their CI fixed by an
  autofix run of the tag (below).
- **Step 9** is the [Minor bump review](SKILL.md#minor-bump-review), once, after
  the highest channel's step 7, so that its picks reach the lower channels too.
- **Step 12** is `publish <tag>`. It brings every branch up to date with its
  channel, runs the presubmit of the branches the run builds (the highest
  channel's, and in an autofix run a lower one's with a fix of its own), and
  pushes none until all are ready. Then it pushes them all, highest first, and
  opens each pull request with `brockit update-version-issue` (unless one is
  open already), which sets the channel's labels and milestone. A channel
  `prepare` left as it is is pushed too, without a rebase, once the run commits
  on it. It leaves the branch it stopped on checked out: handle that as in step
  12 (for exit 2, which only the highest channel asks for: sync, rebuild it,
  then `publish <tag> --built`). With a git cache, `origin` fetches from the
  mirror: check what GitHub has with
  `git ls-remote "$(git remote get-url --push origin)"`, not `origin`.
- **Autofix run of a tag:** `autofix <tag>` resets each bump branch that is on
  GitHub at the tag, and `ci-failures <tag>` writes
  `.autolift/cr<N>.x/ci-failures-<channel>.md` for each pull request. Fix each
  channel's failures on its branch, highest first, as [autofix.md](autofix.md)
  says, building and testing the highest channel as usual. A lower channel that
  needs a fix the higher one made takes it with `channel-picks`, unbuilt: its
  pull request's CI checks it. Build and test a lower channel only for a fix of
  its own, for a failure the higher channel doesn't have;
  `publish <tag> --no-rebase` syncs, presubmits and asks for a rebuild of such a
  branch only.

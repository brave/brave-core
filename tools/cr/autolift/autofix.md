# Autofix run

Read with [SKILL.md](SKILL.md), whose ground rules apply here as in a lift.

A run turns into an autofix run when `prepare` finds nothing worth a lift run
(exit 3, see [Lift target](SKILL.md#lift-target)), and a job can ask for one. It
only fixes what fails in the CI of `cr<N>`'s pull request, on the branch as
GitHub has it: no lift and no rebase, so the commits on GitHub stay as they are,
and fixes go on top as fixups, which the next lift's `brockit rebase` squashes.
The ground rules apply as in a lift. Steps 1 to 12 don't; record what you do in
the report, not with `autolift.py step`. A lift's step 8 follows steps 3 to 5
below, with the differences it lists.

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
   only for what it cuts:
   `vpython3 tools/cr/ci/cli.py log <build URL> --no-timestamps --output .autolift/cr<N>/<name>.log`
   writes a build's whole log, to read with `grep` or `tail -n`, without the
   timestamp on each of its lines, which costs context and rarely tells
   anything. Leave `--no-timestamps` out when the timing matters: for a build
   that timed out or was aborted, a gap between two lines shows where it
   stalled. `vpython3 tools/cr/ci/cli.py checks <branch>`, with the run's branch
   (`cr<N>`), lists every check. It takes a branch or a pull request number, so
   `<N>` alone names another pull request.
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
     Glic tests include suites that don't say Glic, which `-*Glic*` misses:
     those under `chrome/browser/glic/`, those needing Glic for the profile
     (Skills), and those on a `GlicTestEnvironment` ("Not creating
     GlicTestEnvironmentService for ineligible profile" in the log).
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
   - **Tracked in `M<N>.md`**, whatever the label: fix it, as
     [Scope](SKILL.md#scope) says.
   - **Build** (a compile, `gn`, lint or storybook step): reproduce it with the
     narrowest local target, with the flags of [step 3](SKILL.md#3-linux-build)
     (`pnpm run build-storybook` for storybook), and fix it. Build Android, with
     the flags of [step 6](SKILL.md#6-android), only when the failure is in an
     Android check (cr159: `PR browser (android)`); every other failure is
     reproduced and verified on Linux, or fixed from the log for a platform that
     can't be built here.
   - **Broken on the base branch**, whatever the label: leave it, as
     [Scope](SKILL.md#scope) says, and carry on with the other failures.
4. Fix as in a lift, culprit first. Commit each fix as a fixup of the branch
   commit it corrects (`git cr commit --fixup=<sha>`; find it with
   `git log --format='%h %s' <base>..HEAD -- <path>`), or with a pinned subject
   (`IWYU fixes.`, the test exclusions' subjects as in
   [Commits](SKILL.md#commits), and `Version CodeHealth tracker` for the ticked
   task). For a stale plastered patch, that `git log` finds an "Update patches"
   commit: commit it with the subject of the latest "Apply-fixed 🩹 patches"
   commit instead (`git commit -m '<its subject>'`), which the rebase squashes
   into it (see [Commits](SKILL.md#commits)). Only a fix with no commit to fix
   up (a culprit no branch commit cites) is a new `git cr commit --culprit`.
   Platforms that can't be built here (macOS, Windows, iOS) are fixed from the
   log, and listed as unverified.
5. Verify each fix locally: build the targets it touches and run the tests that
   failed, as in [step 4](SKILL.md#4-linux-tests), and with step 6's build and
   tests only for an Android failure. On a fresh out dir, build `brave:all`
   first: a narrow target misses Brave's paks, and browser tests then crash in
   `brave_component_loader.cc` ("invalid Brave Extension manifest") rather than
   where CI failed. Check filter entries by running the reported tests with step
   4's `runtest.py`, which applies every filter that does on CI, and checking
   that none of them runs: `--gtest_list_tests` ignores the filter file, so
   comparing lists shows nothing. Once fixes are committed, run the presubmit as
   in [step 7](SKILL.md#7-presubmit), and address its errors as fixups.
6. Publish without a rebase, in the background, as a bare command (see
   [step 12](SKILL.md#12-publish)):
   `vpython3 tools/cr/autolift/autolift.py publish <N> --no-rebase`. As in
   [step 12](SKILL.md#12-publish), it picks what teammates pushed to `cr<N>`
   during the run and pushes with a lease, but it doesn't catch up with the
   base, rebase or regenerate. Exit 2: the picks changed the branch, so sync
   (`pnpm run sync`), rebuild with [step 3](SKILL.md#3-linux-build)'s command,
   fix what broke as fixups, and run it again with `--built`. Presubmit errors
   stop it as in step 12. It refuses merges, and commits that aren't fixups,
   pinned subjects or the picked tooling: pass `--allow-new` when step 4 made a
   new commit. Conflicts and a rewritten branch are handled as in step 12. The
   push starts CI again; a later run reads the new results.
7. Report each failed check: its builds and label, and what was done (the fix
   commit, the rerun, or why it was left).

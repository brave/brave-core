# Upstream Test Failures

<a id="TUF-001"></a>

## ❌ Never Use Patches to Disable Upstream Test Failures

**Do not create patch files to disable upstream Chromium tests for intermittent
failures.** Use filter files in `test/filters/` instead — they are the preferred
way to disable flaky upstream tests and are trivially reversible.

**Prefer disabling over fixing** intermittent failures in upstream Chromium test
files. Modifying upstream test code via patches is fragile and
maintenance-heavy; a well-documented filter file entry is the correct long-term
solution for upstream flakes. Only fix the upstream test code if Brave's own
`chromium_src` overrides or patches are responsible for the failure.

---

<a id="TUF-002"></a>

## ✅ Check Upstream Flakiness Before Filtering a Test

**Before adding an upstream Chromium test to a filter file, verify that Brave
code is not responsible for the failure.** If a Brave override or patch is
causing the test to fail, fix that code — don't hide it with a filter.

**Run the upstream flake check (from `src/brave`):**

```bash
# Default 30-day lookback
python3 script/check-upstream-flake.py "TestSuite.TestMethod"

# Wider lookback window
python3 script/check-upstream-flake.py "TestSuite.TestMethod" --days 60

# Search by test class name (finds all methods in the suite)
python3 script/check-upstream-flake.py "TestSuite"
```

The script queries Chromium's LUCI Analysis database and returns one of five
verdicts:

| Verdict                          | Flake Rate         | Action                                                   |
| -------------------------------- | ------------------ | -------------------------------------------------------- |
| **Known upstream flake**         | ≥5%                | Safe to filter. Document rate in filter comment.         |
| **Occasional upstream failures** | 1–5%               | Prefer filtering. Document instability.                  |
| **Stable upstream**              | <1%                | Investigate Brave-specific causes before filtering.      |
| **Insufficient data**            | N/A (<10 verdicts) | Manual investigation needed.                             |
| **Not found**                    | N/A                | Test may be Brave-specific or use a different ID format. |

**Flake rate** is calculated as `(failed + flaky) / (passed + failed + flaky)`.

**Decision rule:**

- **≥1% flake rate** → prefer adding to a filter file over attempting a fix
- **<1% flake rate** → investigate Brave-specific causes:
  1. Check `src/brave/chromium_src/` for overrides in the test's directory tree
  2. Check `patches/` for any patch touching the same files

If the flake check or the upstream bug shows the test was already **fixed**
upstream, see [TUF-007](#TUF-007) before writing a filter entry.

---

<a id="TUF-003"></a>

## ✅ Filter Only Specific Flaky Parameterized Variants

**Do not use wildcard filters that suppress stable variants alongside flaky
ones.** Check LUCI Analysis data for each variant individually and only filter
the specific variants that are actually flaky.

```
# ❌ WRONG - wildcard disables all variants including stable ones
-SomeParameterizedTest/*

# ✅ CORRECT - only disable the specific flaky variants
-SomeParameterizedTest/0
-SomeParameterizedTest/2
-SomeParameterizedTest/3
# Variant /1 is stable upstream - keep it enabled
```

---

<a id="TUF-004"></a>

## ✅ Match Filter Specificity to Actual Failure Scope

**Use the most specific/narrow filter approach, but only when the evidence
supports it.** If a test only fails under ASAN on Linux and upstream LUCI
Analysis shows no flakiness on non-ASAN builds, use a
platform-and-sanitizer-specific filter file (e.g.,
`browser_tests-linux-asan.filter`) rather than an all-platform filter. Look at
existing patterns in `build/commands/lib/testUtils.js` for how
sanitizer-specific filters are loaded.

**Do not place a filter in a sanitizer-specific file (e.g., `-asan`, `-msan`) if
upstream LUCI Analysis data shows the test also fails on non-sanitizer builds.**
LUCI Analysis aggregates across all build configs, so a non-zero upstream flake
rate means the test can fail outside of sanitizer builds too. In that case, use
a broader filter file (e.g., `browser_tests.filter` or
`browser_tests-windows.filter`) to cover all environments where the failure may
occur.

---

<a id="TUF-005"></a>

## ✅ Filter File Entries Must Include a Descriptive Comment

**Every disabled test entry in a filter file must be preceded by a comment that
includes:**

1. **Why** the test is disabled
2. **What** specific condition causes the failure
3. **Why** this filter file was chosen (if not obvious from the filename)
4. **Upstream flakiness data** — flake rate and lookback period (for upstream
   Chromium tests)

```
# ❌ WRONG - no explanation
-WebUIURLLoaderFactoryTest.RangeRequest/*

# ✅ CORRECT - full context
# Known upstream flake: 1.8% flake rate over 30 days per LUCI Analysis.
# Mojo data pipe race condition — completion signal arrives before data is
# flushed through the consumer side.
-WebUIURLLoaderFactoryTest.RangeRequest/*
```

---

<a id="TUF-006"></a>

## ✅ Group Filter File Entries by Root Cause

**In filter files, group disabled tests that share a root cause under a single
comment section.** Tests with distinct root causes get their own section. Always
leave a blank line before a new comment section — do not place a comment
immediately after a test entry without a blank line.

```
# ❌ WRONG - mixing causes under one comment, no blank lines
# Various flaky tests
-SuiteA.Test1
# Different issue
-SuiteB.Test2

# ✅ CORRECT - separate sections, blank line before each
# Mojo pipe race condition — data flushed after completion signal.
-SuiteA.Test1
-SuiteA.Test2

# Upstream flake: timing-dependent resource load on slow bots (2.3% / 30d).
-SuiteB.Test3
```

---

<a id="TUF-007"></a>

## ✅ Check Whether Our Pinned Chromium Already Has the Upstream Fix

**When a flaky upstream test has already been fixed upstream, check whether that
fix is in the Chromium revision `package.json` pins before adding a filter
entry.** If it is, there is nothing to filter: the failing CI runs were built
from an older pin, and the filter would silently disable a test that now passes.

**Resolve the pinned revision from `package.json`, never from the local `src/`
checkout.** A local checkout is often behind the pin — or mid-roll — and will
tell you the fix is missing when master already has it.

```bash
# What does master actually pin?
TAG=$(git show upstream/master:package.json |
  python3 -c "import sys, json
print(json.load(sys.stdin)['config']['projects']['chrome']['tag'])")

# Does that exact tag contain the fix? Read the file from gitiles, not src/.
FILE=chrome/browser/.../some_unittest.cc
GITILES=https://chromium.googlesource.com/chromium/src.git
curl -s "$GITILES/+/refs/tags/$TAG/$FILE?format=TEXT" | base64 -d
```

Note that the roll commit's **author date can be weeks older than its commit
date**, because rolls are prepared on a branch and merged later. Use the commit
date (`git log --format=%cd`) when deciding whether a roll predates a failure.

**Confirm which pin a CI failure came from by matching the reported line
number.** A stack trace citing `foo_unittest.cc:262` matches only one revision
of that file. If the failing assertion sits at a different line in the pinned
tag — or that line is a comment there — the failure predates the roll and is
already fixed.

Release branches pin their own Chromium version, so a fix that reaches master
through a major roll does **not** reach `1.xx.x` branches. A flake can be fixed
on master and still be live on beta/release; filtering master is not the remedy
for that, and any uplift is the maintainer's call.

---

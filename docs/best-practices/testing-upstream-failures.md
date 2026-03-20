# Upstream Test Failures

<a id="TUF-001"></a>

## ❌ Never Use Patches to Disable Upstream Test Failures

**Do not create patch files to disable upstream Chromium tests for intermittent failures.** Use filter files in `test/filters/` instead — they are the preferred way to disable flaky upstream tests and are trivially reversible.

**Prefer disabling over fixing** intermittent failures in upstream Chromium test files. Modifying upstream test code via patches is fragile and maintenance-heavy; a well-documented filter file entry is the correct long-term solution for upstream flakes. Only fix the upstream test code if Brave's own `chromium_src` overrides or patches are responsible for the failure.

---

<a id="TUF-002"></a>

## ✅ Check Upstream Flakiness Before Filtering a Test

**Before adding an upstream Chromium test to a filter file, verify that Brave code is not responsible for the failure.** If a Brave override or patch is causing the test to fail, fix that code — don't hide it with a filter.

**Run the upstream flake check (from `src/brave`):**

```bash
# Default 30-day lookback
python3 script/check-upstream-flake.py "TestSuite.TestMethod"

# Wider lookback window
python3 script/check-upstream-flake.py "TestSuite.TestMethod" --days 60

# Search by test class name (finds all methods in the suite)
python3 script/check-upstream-flake.py "TestSuite"
```

The script queries Chromium's LUCI Analysis database and returns one of five verdicts:

| Verdict | Flake Rate | Action |
|---|---|---|
| **Known upstream flake** | ≥5% | Safe to filter. Document rate in filter comment. |
| **Occasional upstream failures** | 1–5% | Prefer filtering. Document instability. |
| **Stable upstream** | <1% | Investigate Brave-specific causes before filtering. |
| **Insufficient data** | N/A (<10 verdicts) | Manual investigation needed. |
| **Not found** | N/A | Test may be Brave-specific or use a different ID format. |

**Flake rate** is calculated as `(failed + flaky) / (passed + failed + flaky)`.

**Decision rule:**
- **≥1% flake rate** → prefer adding to a filter file over attempting a fix
- **<1% flake rate** → investigate Brave-specific causes:
  1. Check `src/brave/chromium_src/` for overrides in the test's directory tree
  2. Check `patches/` for any patch touching the same files

---

<a id="TUF-003"></a>

## ✅ Filter Only Specific Flaky Parameterized Variants

**Do not use wildcard filters that suppress stable variants alongside flaky ones.** Check LUCI Analysis data for each variant individually and only filter the specific variants that are actually flaky.

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

**Use the most specific/narrow filter approach.** If a test only fails under ASAN on Linux, use a platform-and-sanitizer-specific filter file (e.g., `browser_tests-linux-asan.filter`) rather than an all-platform filter. Look at existing patterns in `build/commands/lib/testUtils.js` for how sanitizer-specific filters are loaded.

---

<a id="TUF-005"></a>

## ✅ Filter File Entries Must Include a Descriptive Comment

**Every disabled test entry in a filter file must be preceded by a comment that includes:**

1. **Why** the test is disabled
2. **What** specific condition causes the failure
3. **Why** this filter file was chosen (if not obvious from the filename)
4. **Upstream flakiness data** — flake rate and lookback period (for upstream Chromium tests)

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

**In filter files, group disabled tests that share a root cause under a single comment section.** Tests with distinct root causes get their own section. Always leave a blank line before a new comment section — do not place a comment immediately after a test entry without a blank line.

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

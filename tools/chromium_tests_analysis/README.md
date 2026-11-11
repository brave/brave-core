# Chromium tests analysis

This directory contains scripts for querying Chromium's LUCI Analysis service
about upstream tests that Brave runs on CI:

- `is_upstream_flake.py` asks whether a test is a confirmed flake per variant,
  on the branch the Chromium checkout is at (see
  [Confirmed flakes](#confirmed-flakes)).
- `update-upstream-flake-filters.py` regenerates the filter files in
  `test/filters/generated/` that exclude flaky upstream tests from Brave's test
  runs.
- `luci_analysis.py` is the LUCI Analysis API client used by both scripts.

The rest of this document describes the LUCI data model that the scripts operate
on.

## LUCI

LUCI runs Chromium's tests. Test results are organized by three parameters:

1.  The _test ID_. It uniquely identifies a test in the source code, including
    any parameter values. For example:
    `://base\:base_unittests!gtest::AsanBackupRefPtrTest#Dereference`
2.  The _variant_. Loosely, this captures how the test was built, the
    environment in which it ran (such as the OS version) and the flags it was
    invoked with.
3.  The _invocation_, which is essentially the build. For example,
    `build-8672314611336616241`.

For each such combination, LUCI records a _verdict_, which summarizes all runs
of the test in that invocation (there can be several due to retries). The
verdict can be one of the following:

- _passed_ (all runs passed)
- _failed_ (all runs failed)
- _flaky_ (failed, then passed on retry within the same invocation)

* plus a few other ones such as _skipped_ or _errored_.

### Test ID

The test ID is the fully-qualified form of the familiar test name. It has the
structure `://<target>!<scheme>::<name>`. For the example above:

- `base\:base_unittests` is the build target containing the test
  (`//base:base_unittests` in GN). Structural characters occurring inside a
  component are escaped with a backslash, hence `\:`.
- `gtest` is the _scheme_: the test framework, which defines how the rest of the
  ID is structured. For gtest that is `<suite>#<test>`. Other schemes include
  `webtest` (Blink web tests, where the name is a file path) and `junit`
  (Android Java tests, `package.Class#method`).
- `AsanBackupRefPtrTest#Dereference` is the test name
  `AsanBackupRefPtrTest.Dereference`.

The target prefix matters because the same test name can exist in multiple
targets. For example, `browser_tests` and `android_browsertests` both contain
`IframeInfoMultiSourcePageContextFetcherBrowserTest`, and each has its own ID
and its own results history.

For parameterized gtests, the name components are reordered: the test name
`All/GeolocationBrowserTest.GrantToDenyToGrant/0` (instantiation/suite.test/
parameter) becomes

    ://chrome/test\:browser_tests!gtest::GeolocationBrowserTest#GrantToDenyToGrant/All.0

i.e. `<suite>#<test>/<instantiation>.<parameter>`. This keeps all IDs of a suite
adjacent when sorted.

### Variant

The variant is a set of free-form key/value string pairs, which include
`builder`, `os` and `test_suite`. For example:

    builder: linux-arm64-dbg-tests
    os: Ubuntu-22.04
    test_suite: browser_tests

The keys divide up "how the test ran" as follows:

- `builder` implies the build configuration: compile-time properties such as
  debug vs. release, architecture or sanitizers. These are only encoded in the
  builder's name (e.g. `linux_chromium_asan_rel_ng`), not stored in a structured
  way.
- `os` is the operating system of the machine that ran the test. It can vary
  within one builder, e.g. when the bot fleet migrates to a newer OS version or
  when a builder accepts several (`Mac-15|Mac-26`).
- `test_suite` is the launch configuration: the same binary can be run in
  several modes with different command-line flags (e.g. `browser_tests` vs.
  `browser_tests_no_field_trial`), tracked as separate variants.

Caution: `os` is the machine's OS, not the tested platform. iOS simulator bots
report a Mac `os`, ChromeOS builds run on Ubuntu machines. The builder name is
what disambiguates these.

### Confirmed flakes

Chromium's CQ does not use a flake rate across all of a test's results. To
decide whether a failure is a known flake, it calls
`TestVariants.QueryStability` for one test, one variant and one position on one
branch. LUCI Analysis applies the project's `TestStabilityCriteria`
(`infra/config/luci-analysis.cfg`) to the 14 days of source verdicts around that
position. For Chromium:

- _flaky_: at least 3 source verdicts with both expected and unexpected runs
  within ±7 days, and at least 1 within ±1 weekday.
- _failing_: at least 6 unexpected runs out of 10 around the position, or at
  least 3 consecutive unexpected runs.

`is_upstream_flake.py` makes the same call and reports the answer. Branch
results are stored in a separate LUCI project per milestone: `chromium-m157`
holds `refs/branch-heads/8103`, and `chromium` holds `refs/heads/main`. The
14-day window is always counted back from today, so a branch position older than
that has no data.

### Clusters

LUCI Analysis groups failed test results into _clusters_ (individual runs, so a
flaky verdict's failed attempts are included even though the test eventually
passed). Each failure belongs to several clusters at once:

- One _reason cluster_ with all failures that have a similar failure message.
  Messages are compared after masking volatile parts such as line numbers,
  addresses and values. One reason cluster can span many tests.
- One _test name cluster_ with all failures of the same test. For parameterized
  tests, the failures of all parameter values end up in the same cluster.
- Any number of _rule clusters_. A rule is human-defined (or created by
  automatic bug filing) and ties failures matching a predicate such as
  `reason LIKE "..."` or `test = "..."` to a tracked bug.

Reason and test name clusters are computed automatically and are called
_suggested clusters_: they are suggestions for bugs one might want to file.
Filing (or associating) a bug from a suggested cluster creates a rule for it.
From then on, the rule _claims_ the matching failures: they still belong to
their suggested clusters, but no longer count towards those clusters' failure
counts. A heavily failing test that is already tracked by a bug therefore shows
a test name cluster with a failure count near zero.

#### Limits on reading clusters

Two server-side limits shape what `update-upstream-flake-filters.py` can
discover through clusters. Neither appears in the proto:

- `QueryClusterSummaries` returns at most 200 clusters and does not paginate — a
  bare `LIMIT 200` in `analysis/internal/analysis/cluster_summaries.go`. That
  cap is why the script queries each week of the lookback separately as well as
  the whole of it.
- `QueryClusterFailures` returns at most 2000 failure groups, and **only ever
  reads the last 7 days** whatever lookback the run was given — see
  `ReadClusterFailures` in `analysis/internal/analysis/cluster_failures.go`. So
  `--days 60` widens the flake rates computed for a candidate, but not the
  discovery of candidates through multi-test clusters.

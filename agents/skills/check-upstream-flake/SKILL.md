---
name: check-upstream-flake
description:
  "Check if a failing test is a confirmed upstream flake in Chromium's LUCI
  Analysis, per builder and OS, on the branch Brave is built from. Triggers on:
  check upstream flake, is this test flaky upstream, check luci flakiness,
  upstream flake check."
argument-hint: <TestSuite.TestMethod>
disable-model-invocation: true
---

# Check Upstream Flake

Check if a failing test is a confirmed flake in Chromium upstream. The script
asks LUCI Analysis the question Chromium's CQ asks before exonerating a failure
(`TestVariants.QueryStability`): for one test variant (builder, OS and test
suite) at one position on one branch, do the surrounding days meet Chromium's
flake or failure criteria?

---

## When to Use

- **Investigating intermittent test failures** before deciding on a fix approach
- **Evaluating test disable PRs** to verify upstream flakiness claims
- **During PR review** (via the review skill) when assessing test filter changes
- **Working on "pending" stories** that involve Chromium test failures

---

## The Job

When invoked with a test name:

1. Run the script with `--json` for the test, narrowed to the platform that
   failed if known
2. Read `confirmed` and each entry of `variants` (see
   [JSON output](#json-output)), and report which builders are `flaky` or
   `failing`, on which `ref`
3. Recommend filtering or investigating, per the table below

---

## Usage

```bash
# On the branch the Chromium checkout is at (e.g. refs/branch-heads/8103)
vpython3 ./tools/chromium_tests_analysis/is_upstream_flake.py "WebUIURLLoaderFactoryTest.RangeRequest" --json

# Only Windows bots, or only ASan builders
vpython3 ./tools/chromium_tests_analysis/is_upstream_flake.py "WebUIURLLoaderFactoryTest.RangeRequest" --os windows --json
vpython3 ./tools/chromium_tests_analysis/is_upstream_flake.py "WebUIURLLoaderFactoryTest.RangeRequest" --builder asan --json

# Another Chromium tag or branch, or upstream main
vpython3 ./tools/chromium_tests_analysis/is_upstream_flake.py "WebUIURLLoaderFactoryTest.RangeRequest" --ref 157.0.8103.39 --json
vpython3 ./tools/chromium_tests_analysis/is_upstream_flake.py "WebUIURLLoaderFactoryTest.RangeRequest" --ref main --json
```

Without `--json` the script prints tables for a human reader instead.

**Arguments:**

- `test` (required): gtest name (`Suite.Test`, `Inst/Suite.Test/0`). A name
  without its parameter covers every parameterisation.
- `--ref`: chromium/src tag, branch or commit. Defaults to the local checkout.
- `--builder`, `--os`, `--suite`: case-insensitive regexes on the variant.
- `--all`: also list variants with no results near the position.
- `--json`: output JSON instead of tables.

**Exit codes:**

- `0`: Confirmed flaky or failing upstream on at least one selected variant
- `1`: Error (network, API, unknown ref, etc.)
- `3`: Not confirmed on any selected variant, including when nothing matched

---

## JSON output

```json
{
  "test": "WebUIURLLoaderFactoryTest.RangeRequest",
  "project": "chromium-m157",
  "ref": "refs/branch-heads/8103",
  "position": 1035,
  "commit": "9a31f0acec1854d44e49a73fbf8017598151a583",
  "criteria": {
    "flakeRate": { "flakeThreshold": 3, "flakeThreshold1wd": 1 },
    "failureRate": { "failureThreshold": 6, "consecutiveFailureThreshold": 3 }
  },
  "confirmed": false,
  "variants": [
    {
      "test_id": "://content/test\\:content_unittests!gtest::WebUIURLLoaderFactoryTest#RangeRequest/0",
      "variant": {
        "builder": "linux-rel",
        "os": "Ubuntu-22.04",
        "test_suite": "content_unittests"
      },
      "variant_hash": "b3ba0803668e9981",
      "status": "stable",
      "flake_rate_met": false,
      "failure_rate_met": false,
      "run_flaky_verdicts": 0,
      "total_verdicts": 17,
      "run_flaky_verdicts_1wd": 0,
      "run_flaky_verdicts_12h": 0,
      "unexpected_test_runs": 0,
      "consecutive_unexpected_test_runs": 0,
      "example_builds": []
    }
  ]
}
```

- `confirmed`: true when any variant is `flaky` or `failing`; the exit code says
  the same.
- `variants[].status`: `flaky`, `failing`, `stable` or `no_data`. `--json`
  always includes `no_data` variants; skip them when reporting.
- `variants[].variant`: the builder, OS and test suite the status applies to.
  `bucket: "ci"` marks a CI builder; without it, a CQ trybot.
- `variants[].example_builds`: links to builds where the test was flaky, to cite
  as evidence.

---

## Interpreting Results

| Status      | Meaning                                                                | Action                                                                     |
| ----------- | ---------------------------------------------------------------------- | -------------------------------------------------------------------------- |
| **FLAKY**   | At least 3 flaky verdicts within ±7 days, at least 1 within ±1 weekday | Safe to filter for that platform. Name the builders in the filter comment. |
| **FAILING** | At least 6 of the 10 nearest runs failed, or at least 3 in a row       | Safe to filter. The test is broken upstream on that builder.               |
| **STABLE**  | Results exist and neither criterion is met                             | Investigate Brave-specific causes.                                         |
| **NO DATA** | No results near the position                                           | Cannot determine from upstream data. Try `--ref main` or investigate.      |

A flaky verdict is one where the test failed and then passed on retry in the
same build. The criteria are Chromium's own (`infra/config/luci-analysis.cfg`)
and are printed with every report.

---

## How Results Inform Decisions

### FLAKY or FAILING

- Disabling via filter file is appropriate
- Use the most specific filter file the confirmed builders support
  (platform/sanitizer-specific)
- Include in the filter comment: "Confirmed upstream flake on <branch> per LUCI
  Analysis (FLAKY on <builders>)"
- Reference this in commit message and PR body

### STABLE

- The test passes reliably in Chromium CI on that branch and platform
- Focus investigation on Brave-specific factors:
  - Check `brave/chromium_src/` overrides in related directories
  - Look for Brave features that change timing or behavior
  - Check if Brave adds UI elements that affect the test
- A filter disable should be a last resort and needs strong justification

### NO DATA or nothing matched

- The test may not run on that branch's builders; try `--ref main`
- Check the name: the script matches the gtest name after `::` in the LUCI test
  ID
- Check manually at https://ci.chromium.org/ui/p/chromium/test-search
- Proceed with normal investigation

---

## Limitations

- Only covers Chromium upstream data (not Brave CI)
- LUCI Analysis only keeps the 14 days before today for this query, so a
  checkout older than that has no data
- Release branch results live in a per-milestone LUCI project (`chromium-m157`),
  which runs fewer builders than main
- Does not compare failure logs/output, only pass/fail/flaky outcomes

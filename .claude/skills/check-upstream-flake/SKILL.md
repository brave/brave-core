---
name: check-upstream-flake
description:
  "Check if a failing test is a known upstream flake in Chromium's LUCI Analysis
  database. Provides flakiness statistics, verdict, and recommendation for
  filter file decisions. Triggers on: check upstream flake, is this test flaky
  upstream, check luci flakiness, upstream flake check."
argument-hint: <TestSuite.TestMethod>
---

# Check Upstream Flake

Check if a failing test is a known upstream flake in the Chromium LUCI Analysis
database. This queries the REST API at `analysis.api.luci.app` to retrieve
historical pass/fail/flake data for a test in the Chromium CI infrastructure.

---

## When to Use

- **Investigating intermittent test failures** before deciding on a fix approach
- **Evaluating test disable PRs** to verify upstream flakiness claims
- **During PR review** (via the review skill) when assessing test filter changes
- **Working on "pending" stories** that involve Chromium test failures

---

## The Job

When invoked with a test name:

1. Search for matching test IDs in the Chromium LUCI Analysis database
2. Retrieve flakiness statistics for each match over the lookback period
3. Analyze pass/fail/flake rates
4. Report a verdict and recommendation

---

## Usage

```bash
# Basic: check a specific test (default 30-day lookback)
python3 ./script/check-upstream-flake.py "WebUIURLLoaderFactoryTest.RangeRequest"

# Longer lookback window
python3 ./script/check-upstream-flake.py "WebUIURLLoaderFactoryTest.RangeRequest" --days 60

# JSON output (for programmatic use)
python3 ./script/check-upstream-flake.py "WebUIURLLoaderFactoryTest.RangeRequest" --json

# Search by test class name (finds all methods)
python3 ./script/check-upstream-flake.py "WebUIURLLoaderFactoryTest"
```

**Arguments:**

- `test_name` (required): Test name or substring to search for
- `--days N`: Lookback window in days (default: 30, max: 90)
- `--json`: Output JSON instead of markdown

**Exit codes:**

- `0`: Success (results found and reported)
- `1`: Error (network, API, etc.)
- `2`: No matching test IDs found

---

## Interpreting Results

The script produces one of five verdicts:

| Verdict                          | Flake Rate         | Action                                                                                                                   |
| -------------------------------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------ |
| **Known upstream flake**         | >= 5%              | Safe to add to filter file. Document upstream flakiness in the filter comment.                                           |
| **Occasional upstream failures** | 1-5%               | Consider filtering. Document findings. May still warrant investigation.                                                  |
| **Stable upstream**              | < 1%               | Investigate Brave-specific causes. The test is stable in Chromium, so Brave code changes are likely causing the failure. |
| **Insufficient data**            | N/A (<10 verdicts) | Cannot determine from upstream data. Manual investigation needed.                                                        |
| **Not found**                    | N/A                | Test not in Chromium database. May be Brave-specific or use a different ID format.                                       |

**Flake rate** is calculated as `(failed + flaky) / (passed + failed + flaky)`.
Skipped and precluded verdicts are excluded from the rate.

---

## How Results Inform Decisions

### Known upstream flake or occasional failures

- Disabling via filter file is appropriate
- Use the most specific filter file possible (platform/sanitizer-specific)
- Include in the filter comment: "Known upstream flake (X% flake rate over N
  days per LUCI Analysis)"
- Reference this in commit message and PR body

### Stable upstream

- The test passes reliably in Chromium CI
- Focus investigation on Brave-specific factors:
  - Check `brave/chromium_src/` overrides in related directories
  - Look for Brave features that change timing or behavior
  - Check if Brave adds UI elements that affect the test
- A filter disable should be a last resort and needs strong justification

### Not found or insufficient data

- The test may use a different ID format in LUCI
- Try searching with just the class name or a broader substring
- Check manually at https://ci.chromium.org/ui/p/chromium/test-search
- Proceed with normal investigation

---

## API Details

The script uses the LUCI Analysis REST API (pRPC protocol):

- **QueryTests**:
  `POST https://analysis.api.luci.app/prpc/luci.analysis.v1.TestHistory/QueryTests`
- **QueryStats**:
  `POST https://analysis.api.luci.app/prpc/luci.analysis.v1.TestHistory/QueryStats`
- **Query** (fallback):
  `POST https://analysis.api.luci.app/prpc/luci.analysis.v1.TestHistory/Query`

No authentication is required for public Chromium data.

Test IDs in LUCI follow the format:
`ninja://{gn_path}:{target}/{TestSuite}.{TestMethod}`

---

## Limitations

- Only covers Chromium upstream data (not Brave CI)
- Test ID format may not match for all tests
- Historical data limited to ~90 days
- Does not compare failure logs/output (only counts pass/fail/flake)
- Cannot distinguish between different failure modes for the same test

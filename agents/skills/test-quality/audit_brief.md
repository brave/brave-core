# Test-quality audit brief

Audit Brave's **{TYPE_NAME}** for weak tests. This is a READ-ONLY audit.

- Do NOT modify any repo file, build, run tests, use `gh`, or post anything.
- Your only writes are to `{DIR}/audit/results/shard-NN.jsonl`.

Repo root (cwd): `{REPO_ROOT}` (Brave customizations). Chromium is the parent
dir `src/`. Your file list is `{DIR}/audit/shards/shard-NN.txt` (tab-separated:
path relative to the repo root, line count, test count). Review **every** file
in it.

## The question

For each test: **"If the base functionality this test is meant to cover broke,
would the test fail?"** Flag tests where the answer is NO (or only for a trivial
subset of the behavior).

Do not judge from the test alone. Read the production code under test, then
mentally mutate it: flip a condition, delete a branch, return a wrong/default
value, skip a call, swap arguments, drop a pref/field write, off-by-one. If a
realistic mutation leaves the test passing, that's a finding.

## Categories (use these slugs)

- `no-assertions` — nothing in the body depends on production behavior (only
  constructs, or runs and checks nothing) while the name claims behavior.
  Crash-only/robustness tests that say so are fine.
- `tests-the-mock` — assertions only verify values/calls the test itself set up.
- `production-code-stubbed` — the logic under test is replaced by a fake/mock/
  override so the real code doesn't run.
- `unreached-assertions` — assertions live in callbacks/observers/lambdas never
  confirmed to run (no RunLoop quit, TestFuture, or called-flag check), or run
  before posted/async work is pumped, so the test passes if it never happens.
- `loose-assertions` — wrong output would still pass (`EXPECT_FALSE(x.empty())`,
  `EXPECT_NE(nullptr, p)`, `has_value()` without checking the value, only one of
  several outputs checked, mocked responses that answer any URL so the request
  is never checked).
- `tautology` — expected value computed with the same production code/constant
  or a copy of the implementation; comparing an object with itself.
- `name-body-mismatch` — name/comment claims behavior X, body doesn't exercise X
  (e.g. a negative case that fails earlier for a different reason, so the guard
  it claims to test is never reached).
- `missing-key-case` — a core branch (error/disabled/negative path) of the code
  the file claims to cover is exercised by NO test in the file, so deleting it
  fails nothing. Use sparingly; this is not a general coverage hunt.
- `disabled-or-skipped` — `DISABLED_`, `GTEST_SKIP`, `#if 0`, commented-out
  body.
- `other` — explain.

## Test-type notes ({TYPE})

- **unittest:** watch for fixtures that pre-set the very state the test claims
  to verify, mocks returning canned data for any input, and `Times(0)` on mojo
  mocks whose messages are never delivered because the loop isn't pumped.
- **browsertest / interactive_uitest:** watch for checks that run before the
  navigation/load/JS completes, `EvalJs` results never compared, waits that pass
  on timeout, `PRE_` tests whose follow-up doesn't check the persisted state,
  and tests that only assert the browser didn't crash.
- **webui:** watch for snapshot-only tests, tests that assert on mocked API
  return values, and handlers that are mocked so the component logic never runs.

## Precision rules

- Precision over recall. Report only if you can name a **concrete, realistic
  production change** the test would survive, with a production `path:line`.
- Don't flag a test for being short/simple, or for mocking _dependencies_ (fine
  when assertions check the unit's own behavior).
- "No test for feature Y" is coverage, not weakness — skip (except
  `missing-key-case` above).
- Table/parameterized tests: judge the table's discriminating power.
- Confidence: `high` = you traced production code and a mutation passes;
  `medium` = plausible with some uncertainty. Skip anything lower.

## Process

For each file: read it fully (offset/limit for huge files), read the production
code, decide. After finishing **each file**, append ONE JSON line for it to
`{DIR}/audit/results/shard-NN.jsonl` with Bash and a single-quoted heredoc
(progress must survive context compaction). Do this even for files with zero
findings.

```
cat >> {DIR}/audit/results/shard-NN.jsonl <<'EOF'
{"path": "...", "tests_reviewed": 12, "weak": 2, "strong": 10, "findings": [ ... ]}
EOF
```

Finding schema:

```
{"tests": ["Suite.Name", ...],      // group tests sharing a root problem
 "line": 123,                       // line of the first test
 "category": "<slug>",
 "confidence": "high|medium",
 "summary": "<one sentence: why this test wouldn't catch a break>",
 "regression": "<concrete production change that would still pass>",
 "production_ref": "path:line",
 "suggested_fix": "<1-2 sentences>"}
```

`tests_reviewed` = test cases you actually reviewed; `weak` = distinct tests
covered by any finding; `strong` = tests_reviewed − weak.

When done, validate:
`python3 -I -c 'import json,sys; [json.loads(l) for l in open(sys.argv[1])]' {DIR}/audit/results/shard-NN.jsonl`

## Final reply (under 150 words)

Shard id, files done / total, tests reviewed, weak-test count, your 3 most
compelling findings (one line each), and any files not finished. Do not paste
the JSON.

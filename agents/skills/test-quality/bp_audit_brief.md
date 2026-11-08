# Test best-practices audit brief

Audit Brave's **{TYPE_NAME}** against the project's documented test best
practices. This is a READ-ONLY audit.

- Do NOT modify any repo file, build, run tests, use `gh`, or post anything.
- Your only writes are to `{DIR}/audit/results/shard-NN.jsonl`.

Repo root (cwd): `{REPO_ROOT}` (Brave customizations). Chromium is the parent
dir `src/`. Your file list is `{DIR}/audit/shards/shard-NN.txt` (tab-separated:
path relative to the repo root, line count, test count). Review **every** file
in it.

## The question

For each test file: **does it break any rule in `{DIR}/PRACTICES.md`?** Read
that file first, in full. It holds the rules that apply to {TYPE_NAME}, with the
rule text from `docs/best-practices` and an **audit note** that narrows what
counts as a violation here. It also lists preferred patterns, which you use only
to word `suggested_fix`.

This lane checks rules, not whether a test would catch a regression (another
lane judges that by mutation). Report only what a listed rule names. A test
being weak, short or untested is not a finding.

## What to report

- Every finding names one rule id from "Rules to audit", quotes the offending
  code, and says how it breaks the rule. No rule id, no finding.
- Read the whole file, including the fixture, `SetUp`/`TearDown`, helpers and
  base classes it relies on: a rule is not broken when a base class already
  restores the global, pins the feature, or asserts in the helper. Read
  production code only as far as a rule needs it (e.g. TI-027, BS-033, BS-023).
- **One finding per (rule, file).** List every affected test in `tests`; use
  `[]` when the problem is in the fixture or file scope. Use the first offending
  line for `line`.
- Only code in this file counts. A `chromium_src` test override that `#include`s
  the upstream test file does not own the upstream tests; flag only Brave's own
  lines.
- Follow each rule's own exceptions and its audit note. If the documented
  alternative cannot be used here (no callback to capture, nothing to observe),
  don't report it.
- Precision over recall. Confidence: `high` = the rule text plainly covers the
  code and you checked the surrounding code; `medium` = a judgment call. Skip
  anything lower.
- Don't invent rules. Anything that is merely your preference is not a finding.

## Test-type notes ({TYPE})

- **unittest:** many files are upstream Chromium tests that Brave overrides or
  extends; judge Brave's lines only. Component tests live under `components/`;
  tests under `browser/` and `ios/` have a different dependency budget.
- **browsertest / interactive_uitest:** most timing and JS rules apply. Read the
  wait helpers a test calls before deciding it doesn't wait.
- **webui:** Jest/Testing Library tests; few rules apply, so most files will
  have no findings. That is expected.

## Process

For each file: read it fully (offset/limit for huge files), decide. After
finishing **each file**, append ONE JSON line for it to
`{DIR}/audit/results/shard-NN.jsonl` with Bash and a single-quoted heredoc
(progress must survive context compaction). Do this even for files with zero
findings.

```
cat >> {DIR}/audit/results/shard-NN.jsonl <<'EOF'
{"path": "...", "tests_reviewed": 12, "flagged": 2, "clean": 10, "findings": [ ... ]}
EOF
```

Finding schema:

```
{"tests": ["Suite.Name", ...],      // every affected test; [] if file-wide
 "line": 123,                       // first offending line
 "rule": "TA-005",                  // id from "Rules to audit"
 "confidence": "high|medium",
 "summary": "<one sentence: how this code breaks the rule>",
 "evidence": "<the offending code, verbatim, at most 2 lines>",
 "suggested_fix": "<1-2 sentences, naming the documented alternative>"}
```

`tests_reviewed` = test cases in the file; `flagged` = distinct tests covered by
any finding (count a file-wide finding as 0); `clean` = tests_reviewed −
flagged.

When done, validate:
`python3 -I -c 'import json,sys; [json.loads(l) for l in open(sys.argv[1])]' {DIR}/audit/results/shard-NN.jsonl`

## Final reply (under 150 words)

Shard id, files done / total, tests reviewed, flagged-test count, findings per
rule, your 3 most compelling findings (one line each), and any files not
finished. Do not paste the JSON.

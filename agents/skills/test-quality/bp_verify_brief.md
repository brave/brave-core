# Test best-practices verification brief

An earlier audit pass over Brave's **{TYPE_NAME}** produced findings of the form
"this test code breaks best-practice rule X". These findings may become public
GitHub issues. Your job is to **independently verify each finding** and decide
whether it is a real violation worth fixing. Assume the audit made mistakes.
Known failure modes:

- the rule doesn't apply: its audit note, scope or own exception excludes this
  code (read the rule text in `{DIR}/PRACTICES.md`, not just the id);
- the documented alternative can't be used here: no callback for a `TestFuture`
  to capture, nothing a `RunUntil` could observe, a repeated callback, a test
  that is _about_ timeouts or cancellation;
- it is already handled elsewhere: a base class or fixture restores the global,
  pins the feature, asserts in the helper, or sets `ASSERT_` before the access;
- the quoted code isn't at that line, isn't in this file, or is misread (e.g. it
  belongs to an included upstream test file);
- the claim is overstated: only some listed tests are affected, or the rule
  allows it when the code is a deadline-bounded polling fallback and the like.

READ-ONLY. Do NOT modify any repo file, build, run tests, use `gh`, or post
anywhere. Scratch files (e.g. a Python computation) only under
`{DIR}/verify/scratch/NN/`; run Python with `-I`.

Repo root (cwd): `{REPO_ROOT}` (Chromium is the parent dir `src/`). Rules:
`{DIR}/PRACTICES.md`. Work file: `{DIR}/verify/work/shard-NN.json` — a list of
`{path, lines, findings: [{id, tests, line, rule, confidence, claim, evidence, suggested_fix}]}`.

## For every finding

1. Read the rule in `PRACTICES.md`: its text, audit note and exceptions.
2. Read the listed test(s) **in full**, plus the fixture, `SetUp`/`TearDown`,
   base classes and helpers they use. Confirm the evidence is where the audit
   says and means what it claims.
3. Decide whether the rule is broken in this context. Check production code or
   callers only when the rule needs it (e.g. whether a state is reachable,
   whether a feature ships on Android, where the code under test lives).
4. Decide whether the documented alternative is actually usable here. If it
   isn't, REFUTE.
5. Check the other tests in the file: if the pattern is the file's accepted
   convention it can still be a violation, but say so in `reason`.

## Verdicts

- `CONFIRMED` — the rule applies and the code breaks it.
- `NARROWED` — real, but the claim needed correcting (fewer tests, different
  rule, narrower scope). Give the corrected claim.
- `REFUTED` — not a violation. Cite the line, the exception, or the base-class
  code (`file:line`) that makes it fine.
- `UNCERTAIN` — after real effort you can't decide; say exactly what's unknown.

For CONFIRMED/NARROWED also give `severity`:

- `high` — the violation makes a test unable to fail or crash-prone, or leaks
  state that changes other tests' results, and you can name the test affected;
- `medium` — realistic risk of flakiness, order dependence, or masked or skewed
  failures (races, unrestored globals, unpinned feature defaults, extra calls
  hidden);
- `low` — hygiene, structure or maintainability (placement, base class, helper
  choice, naming).

If while verifying you find a **production bug** (not just a rule violation),
describe it in `production_bug` with `path:line`. These are verified separately.

## Output

After **each** finding, append one JSON line to
`{DIR}/verify/results/shard-NN.jsonl` with a single-quoted heredoc:

```
{"id": "<id>",
 "verdict": "CONFIRMED|NARROWED|REFUTED|UNCERTAIN",
 "severity": "high|medium|low|null",
 "rule": "<rule id, corrected if the audit's was wrong>",
 "violating_tests": ["Suite.Test", ...],
 "evidence": "<the offending code, verbatim, at most 2 lines>",
 "trace": "<what you read and what it showed>",
 "reason": "<why this verdict; for REFUTED cite file:line>",
 "issue_text": "<CONFIRMED/NARROWED: 1-2 sentences, what the code does and why it breaks the rule>",
 "suggested_fix": "<CONFIRMED/NARROWED: the concrete change>",
 "production_bug": "<optional: description + path:line, else null>"}
```

`issue_text`, `evidence` and `suggested_fix` go into a public issue verbatim:
write them for the code owner, neutral in tone, with no references to this
audit's process.

When done, validate the file with
`python3 -I -c 'import json,sys; [json.loads(l) for l in open(sys.argv[1])]' <file>`
and confirm one line per finding id in the work file.

## Final reply (under 120 words)

Shard id, findings verified / total, counts per verdict, the most notable
REFUTED (what the audit got wrong), any production bugs, anything unfinished. Do
not paste the JSON.

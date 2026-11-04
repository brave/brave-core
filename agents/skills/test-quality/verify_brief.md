# Test-quality verification brief

An earlier audit pass over Brave's **{TYPE_NAME}** produced findings of the form
"test X would still pass if production behavior Y broke". These findings may
become public GitHub issues. Your job is to **independently verify each
finding** and decide whether it is a real problem. Assume the audit made
mistakes. Known failure modes:

- misread the test (missed an assertion, a fixture/SetUp/TearDown check, a
  helper that asserts, a StrictMock / `EXPECT_CALL` default, a `TestFuture` that
  would hang or time out, a death test, a DCHECK/CHECK that fires in tests);
- the proposed mutation would actually trip some assertion, wouldn't compile, or
  is a no-op;
- **another test elsewhere already catches it** (unit, browser, iOS/Android, or
  an upstream Chromium test for a `chromium_src` override) — this was the most
  common audit error;
- the behavior isn't reachable or isn't core: the only production caller
  validates first, the code has no production caller, the field is never read,
  log-only/defensive code;
- the claim is right in spirit but overstated (only some listed tests are weak;
  the mutation is narrower than claimed; the suggested fix would break an
  existing passing case).

READ-ONLY. Do NOT modify any repo file, build, run tests, use `gh`, or post
anywhere. Scratch files (e.g. a Python computation) only under
`{DIR}/verify/scratch/NN/`; run Python with `-I`.

Repo root (cwd): `{REPO_ROOT}` (Chromium is the parent dir `src/`). Work file:
`{DIR}/verify/work/shard-NN.json` — a list of
`{path, lines, findings: [{id, tests, line, category, confidence, claim, mutation, production_ref, suggested_fix}]}`.

## For every finding

1. Read the listed test(s) **in full**, plus the fixture, SetUp/TearDown and any
   helpers they call.
2. Read the production code at `production_ref` and around it, and find its
   production callers.
3. **Simulate the mutation.** Mentally apply it and walk through each listed
   test's execution step by step. List every assertion or expectation that runs,
   and decide whether any would fail.
4. **Search the whole repo** for other tests that would catch the mutation:
   `git grep -n` on the function name, distinctive constants/strings, the
   branch's error message or enum value, across `*unittest*`, `*browsertest*`,
   `*_test.cc`, `test/` dirs, iOS `.mm` tests and Android/Java tests where
   relevant. Read the plausible candidates.
5. Decide whether the behavior matters (user-visible result, security/privacy,
   money/keys, persistence/deletion, protocol/serialization, crash, or error
   handling callers rely on).

## Verdicts

- `CONFIRMED` — the mutation (or a corrected one you state) passes every test in
  the repo and the behavior matters.
- `NARROWED` — real, but the claim needed correcting. Give the corrected claim.
- `REFUTED` — not a real problem. Cite the assertion (`file:line`) that would
  fail, the covering test (`file:Suite.Test`), or why it isn't meaningful.
- `UNCERTAIN` — after real effort you can't decide; say exactly what's unknown.

For CONFIRMED/NARROWED also give `severity`:

- `high` — security/privacy decision, money/keys/funds, data loss or wrongful
  deletion, crash, or a core user-visible feature silently broken;
- `medium` — functional correctness of a real feature;
- `low` — edge case, cosmetic, off-by-default/unwired feature, or test hygiene.

If while verifying you find a **production bug** (not just a weak test),
describe it in `production_bug` with `path:line`. These are verified separately.

## Output

After **each** finding, append one JSON line to
`{DIR}/verify/results/shard-NN.jsonl` with a single-quoted heredoc:

```
{"id": "<id>",
 "verdict": "CONFIRMED|NARROWED|REFUTED|UNCERTAIN",
 "severity": "high|medium|low|null",
 "category": "<audit category slug, corrected if the audit's was wrong>",
 "weak_tests": ["Suite.Test", ...],
 "mutation": "<the mutation you verified passes (CONFIRMED/NARROWED)>",
 "production_ref": "path:line",
 "trace": "<assertions that run under the mutation and why each still passes>",
 "search": "<searches run and what they found; why no other test catches it>",
 "reason": "<why this verdict; for REFUTED cite file:line or file:Suite.Test>",
 "issue_text": "<CONFIRMED/NARROWED: 1-2 sentence why-weak, accurate and neutral>",
 "suggested_fix": "<CONFIRMED/NARROWED: the concrete test case(s) to add>",
 "production_bug": "<optional: description + path:line, else null>"}
```

`issue_text`, `mutation` and `suggested_fix` go into a public issue verbatim:
write them for the code owner, with no references to this audit's process.

When done, validate the file with
`python3 -I -c 'import json,sys; [json.loads(l) for l in open(sys.argv[1])]' <file>`
and confirm one line per finding id in the work file.

## Final reply (under 120 words)

Shard id, findings verified / total, counts per verdict, the most notable
REFUTED (what the audit got wrong), any production bugs, anything unfinished. Do
not paste the JSON.

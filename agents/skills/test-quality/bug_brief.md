# Production-bug verification brief

Agents auditing Brave's tests noticed suspected **production bugs**. Several may
be wrong or overstated. Decide for each whether it's a real bug. READ-ONLY:
don't modify any repo file, build, run tests, use `gh`, or post anywhere. Run
any Python with `-I`, and put scratch files only under `{DIR}/bugs/scratch/`.

Repo root (cwd): `{REPO_ROOT}` (Chromium is the parent dir `src/`; mojo bindings
and base are there).

Input: `{DIR}/bugs/candidates.json` — a list of `{from_finding, claim}`.

For each candidate:

1. Read the cited production code and enough of its callers/callees to trace the
   real control flow. Don't trust the claim's line numbers or reasoning;
   re-derive it.
2. Decide whether the triggering input can occur in production: who calls this,
   with what data; is it behind a default-off feature; is the code reachable at
   all; is the input trusted/self-written or from a server, web page or another
   process.
3. For claims about Chromium/mojo/libc++ behavior (non-nullable mojo params,
   `VLOG` evaluation, hardened libc++ asserts, `base::Value` accessors), check
   the actual Chromium source under `../` rather than assuming.
4. For "spec mismatch" claims, state both readings and which the code, comments
   and feature defaults support.
5. Skip candidates that duplicate one already judged (same defect).

Verdict per candidate: `REAL` (confirmed, reachable), `LATENT` (real defect but
not reachable today / no production caller / default-off), `NOT_A_BUG` (claim
wrong; say why), or `UNSURE`. Severity for REAL/LATENT: `high` (crash reachable
from untrusted input, security/privacy, funds/keys, data loss), `medium`
(user-visible functional breakage), `low`.

After each candidate, append one JSON line to `{DIR}/bugs/verified.jsonl` with a
single-quoted heredoc:

```
{"from_finding": "...", "verdict": "REAL|LATENT|NOT_A_BUG|UNSURE",
 "severity": "high|medium|low|null", "location": "path:line",
 "title": "<short bug title>",
 "explanation": "<2-4 sentences: the defect, the trigger, the effect>",
 "reachability": "<who reaches it and with what input>",
 "fix": "<1 sentence>", "evidence": "<files/lines you read>"}
```

`title`, `explanation`, `reachability` and `fix` go into a public issue
verbatim: write them for the code owner, with no references to the audit.

Validate the file with
`python3 -I -c 'import json,sys; [json.loads(l) for l in open(sys.argv[1])]' <file>`.

Final reply (under 150 words): counts per verdict and a one-line summary for
each REAL/LATENT bug, highest severity first.

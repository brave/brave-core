---
name: chromium-bump-secpriv
description:
  'Security & privacy review of a Chromium upgrade PR in brave/brave-core.
  Inventories security-sensitive changes (IPC/mojo, script injection, removed
  upstream CHECKs), audits new telemetry end-to-end (collection → hashing →
  storage → P3A transmission), and checks required signoffs (sec-team,
  chromium-src-web-reviewers, p3a Slack privacy review). Triggers on: chromium
  bump security review, bump privacy review, bump secpriv, chromium upgrade
  security, review bump security.'
argument-hint: <pr-number-or-url>
disable-model-invocation: true
---

# Chromium Bump Security & Privacy Review

Security/privacy pass over a Chromium upgrade PR
(`Upgrade from Chromium X to Chromium Y`). Complements the `review` skill
(best-practices/code review) — this skill covers only security & privacy.

## When to Use

- Reviewing a Chromium version bump PR (`Upgrade from Chromium X to Y`)
- Checking whether required bump signoffs (sec-team, p3a, QA) are in place
- Auditing new telemetry that rode along with the bump

Method distilled from:

- Brave handbook (`brave/handbook` → `development/code-review-guidelines.md`)
- brave-browser wiki
  [Security-reviews](https://github.com/brave/brave-browser/wiki/Security-reviews)
- Previous bump reviews (e.g. #37709 151→152, #38561 152→153)
- docs/best-practices (`architecture.md#ARCH-068`, `chromium-src-overrides.md`,
  `patches.md`, `build-system.md`)

---

## Step 1 — Context & Scope

```bash
PR=<number-or-URL>; REPO=brave/brave-core
gh pr view $PR --repo $REPO --json title,body,state,mergeable,headRefName,headRefOid
HEAD=$(gh pr view $PR --repo $REPO --json headRefOid -q .headRefOid)
gh pr diff $PR --repo $REPO > /tmp/bump.diff
git fetch origin HEAD_HERE=<branch>   # then validate via `git show $HEAD:<path>`
BASE=$(git merge-base origin/master $HEAD)
```

Notes:

- `gh pr diff` returns 406 with large diffs → fetch branch locally and use
  `git diff $BASE..$HEAD` instead.
- `compare-chromium-versions` check lists upstream delta; confirm base branch
  and that EXTRA_DEPS toolchain pins (e.g. rust/llvm `llvmorg-XX-init`) changed
  consistently with their sha256/digests.

Classify the diff — only these areas get the deep security/privacy pass:

| Area                                                           | Why it matters                |
| -------------------------------------------------------------- | ----------------------------- |
| `patches/*` touching IPC/mojo/webui/renderers                  | can remove upstream hardening |
| `chromium_src/*` for `content/**`, `*webui*`, `mojo*`          | security boundary code        |
| new mojoms, `associated_interfaces`, URLLoaderFactory/Proxying | IPC surface                   |
| `ExecuteScript`/`RequestExecuteScript`/`EvalJs` additions      | renderer code injection       |
| new P3A/UMA metrics, new WebUIs, hidden WebContents            | telemetry / collection        |
| new endpoints in constants/GRD, HSTS list                      | network surface               |

The rest (test filters, version bumps, DEPS, reformat/reindex-only patches)
needs only Steps 3 & 6.

---

## Step 2 — Security-Sensitive Change Inventory

```bash
# renderer code injection (reviewdog also flags these)
grep -nE '^\+.*(RequestAsyncExecuteScript|RequestExecuteScript|ExecuteScript|ExecJs|EvalJs)' /tmp/bump.diff
# IPC surface
grep -nE '^\+.*(\.mojom|AssociatedRemote|Remote<|GetRemoteAssociatedInterfaces|URLLoaderFactory|ProxyingURLLoaderFactory)' /tmp/bump.diff
# hardware/permission surfaces
grep -nE '^\+.*(Hid|Usb|Serial|Bluetooth|Clipboard|PermissionsProfile)' /tmp/bump.diff
# new privileged surfaces
grep -nE '^\+.*(WebUIConfig|RegisterWebUIConfig|WebContents::Create|ISOLATED_WORLD)' /tmp/bump.diff
```

For each hit, read the code at head (`git show $HEAD:<path>`) and answer:

1. Who can reach this path? (user page / WebUI / brave-created WebContents)
2. Is input validated / origin-checked?
3. Interface scoped to one frame/WebContents or globally bindable?

---

## Step 3 — Removed Hardening in Patches (highest-value check)

Bumps routinely lose Brave-added lines or upstream hardening while re-basing
hunks. Inspect removed (`-`) lines in `patches/`:

```bash
git diff $BASE..$HEAD -- patches/ | grep -B3 -E '^-.*(CHECK|DCHECK|NOTREACHED|clear\(\)|InvalidateWeakPtrs|reset\(\))'
git diff $BASE..$HEAD -- patches/ | grep -E '^-' | grep -iE 'brave|BRAVE_' | head -50
git diff $BASE..$HEAD -- rewrite/ | grep -E '^-.*replace' | head -50   # lost plaster replacements
```

Escalate to sec-team when a patch/plaster:

- removes upstream `CHECK`/`DCHECK`/`NOTREACHED` or downgrades a crash to an
  error-return (example class: `mojo_facade.mm` CHECK removal in #38561)
- deletes state resets between page loads/navigations (pipes, watchers,
  observers, `InvalidateWeakPtrs`)
- flips sanitizer/feature defaults in `*switches`-style files

Each removal needs a documented threat-model justification in the PR — "bump
reindex" is not one. Also verify fixes reviewers already discussed actually
landed (check resolved threads, e.g. per-host facade redesign + test in #38561)
— but still require explicit sec-team signoff.

---

## Step 4 — New Telemetry: Privacy Audit

```bash
git diff $BASE..$HEAD --stat -- '**/misc_metrics/' '**/p3a*' '**/metrics*'
grep -nE '^\+.*(p3a_utils::|Record.*Histogram|UmaHistogram|P3A|kHistogramName|HistogramName)' /tmp/bump.diff
```

For every new metric surface, audit end-to-end and output the data-flow table:

| Stage        | Question                                                                                 |
| ------------ | ---------------------------------------------------------------------------------------- |
| Collection   | What data, from where (user pages? hidden WebContents? privileged pages)?                |
| Sanitization | Hashed/salted/aggregated before leaving collector? Read the JS/C++ — don't trust names.  |
| Storage      | `local_state` vs profile pref? Synced? Retention/overwrite/clear policy?                 |
| Transmission | What goes on the wire — raw values, hashes, only aggregate buckets?                      |
| Consent      | Gated by P3A framework consent? Feature `DISABLED_BY_DEFAULT`? Incognito/guest excluded? |

Verify stated invariants in code, e.g. _"fingerprint values are never
transmitted"_ (#38561 fingerprint probe): confirmed hashing in-renderer + C++
stores/reports only hashes, and probe script egress checked (no `fetch(`, XHR,
WebSocket, `sendBeacon`). That's the bar — execute every audit to that level,
not just to "looks fine".

Severity examples: unsalted weak hash (fine for change detection,
brute-forceable if local_state exfiltrates → low, on-device only); test-only
bypass branches in production collectors (medium); script injection into a WebUI
(confirm interface only bindable from Brave-created WebContents — escalate to
sec-team).

**Process gate**: new P3A metrics require a privacy-review post in the p3a Slack
channel, linked in the PR
([docs/best-practices/architecture.md#ARCH-068](https://github.com/brave/brave-core/tree/master/docs/best-practices/architecture.md#ARCH-068)).
This audit does not replace that review.

---

## Step 5 — Network Surface

```bash
git diff $BASE..$HEAD | grep -E '^\+.*https?://' | grep -vE 'test|example|source.chromium|chromium.googlesource|issues.chromium|github.com'
git diff $BASE..$HEAD -- '**/hsts*' '**/*pin_list*'
```

New Brave-owned domains → check the network-audit allowed list
(`browser/net/brave_static_redirect_network_util.cc` + audit test) and HSTS pin
list per build-system best practices. Also scan `.grd`/`.grdp` for new remote
hosts.

---

## Step 6 — Process Gates & Signoffs (blocking)

From the handbook (`development/code-review-guidelines.md`) and previous bump
PRs (#37709 pattern), a bump PR needs **all** of:

1. **sec-team review** on the PR (the `sec-team` reviewer set); link the wiki
   Security-reviews page. Determine the current required team reviewers from
   recent merged bump PRs, not hard-coded names.
2. **chromium-src-web-reviewers** signoff for `chromium_src`/web-exposed
   changes.
3. **p3a Slack privacy review link** for each new metric family (ARCH-068).
4. **QA regression pass clean** — scan review comments for functional
   regressions (e.g. #38561 desktop "Tabs from other devices" broken). An open
   QA regression blocks merge.
5. **CI green + approvals newer than head** — all TeamCity/Jenkins checks pass
   on current head; approve-stamp-review timestamps must postdate the latest
   push (re-request review otherwise).
6. **Merge commit, not squash** — handbook rule for Chromium upgrades.
7. Reviewdog/presubmit security flags resolved or explicitly waived (e.g.
   `RequestExecuteScript` usages).

Check status:

```bash
gh api repos/$REPO/pulls/$PR/reviews --paginate --jq '.[] | {user: .user.login, state, submitted_at}' | sort -u
gh pr checks $PR --repo $REPO
```

---

## Step 7 — Report

```markdown
# Chromium Bump Sec/Priv Review: #<number> (X → Y)

## Security escalations # file/line, hardening delta, why risky, required justification

## Privacy audit # data-flow table per metric + findings (severity)

## Network surface # endpoints / list checks

## Process gates # gate | status | evidence (who/where)

## Blocking items # ordered list for the author
```

Verdict rules:

- Removed hardening without documented justification → **FAIL** @sec-team.
- New telemetry without p3a Slack link → **FAIL** (process gate).
- Open QA regression → **FAIL** until fixed.
- Clean but signoffs pending → **NOT READY** (sec-team / chromium-src-web / CI).

## Completion Checklist

- [ ] Diff fetched (or branch cloned) and classified per Step 1 table
- [ ] Step 2 greps run; each hit read at head commit
- [ ] Step 3 removed-hardening greps run (patches/ AND rewrite/)
- [ ] Every new metric got the full data-flow audit table
- [ ] Stated privacy invariants verified in code (not just names)
- [ ] Network egress greps run; audit list/HSTS checked if needed
- [ ] All 7 process gates checked with evidence (file/line/timestamps)
- [ ] Report sections ordered; blocking items enumerated
- [ ] No GitHub posting without explicit ask (review skill posting flow if so)

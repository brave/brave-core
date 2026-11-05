#!/usr/bin/env python3

# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Bookkeeping for the test-quality skill.

Agents do the judging; this script enumerates and shards the tests, tracks
progress, merges results into issue drafts, and posts them rate limited.

    tq.py init --type unittest [--lane LANE] [--path components/brave_wallet]
    tq.py status DIR
    tq.py verify-work DIR
    tq.py bug-work DIR
    tq.py aggregate DIR [--min-severity low|medium|high]
    tq.py labels --repo brave/brave-browser
    tq.py post DIR --account netzenbot (--one PATH | --batch | --bugs)

`init` makes one work dir per lane under its output dir: `effectiveness` (would
the test fail if the code broke) and `best-practices` (does the test follow the
documented test rules). `status`, `verify-work`, `bug-work` and `aggregate` take
either the output dir (all lanes) or one lane dir; `post` takes a lane dir.
"""

import argparse
import collections
import datetime
import json
import os
import random
import re
import subprocess
import sys
import tempfile
import time
from pathlib import Path

_SKILL_DIR = Path(__file__).resolve().parent
# .../src/brave/agents/skills/test-quality/tq.py -> parents[3] == .../src/brave
_REPO_ROOT = Path(__file__).resolve().parents[3]

_GTEST_RE = (
    r'^\s*(TEST|TEST_F|TEST_P|TYPED_TEST|TYPED_TEST_P|'
    r'IN_PROC_BROWSER_TEST|IN_PROC_BROWSER_TEST_F|'
    r'IN_PROC_BROWSER_TEST_P)\('
)
_JS_RE = r'^\s*(it|test)(\.each\(.*?\))?\s*\('

# type -> (display name, file regex, test-case regex)
TYPES = {
    'unittest': ('unit tests', r'_unittest\.(cc|mm)$', _GTEST_RE),
    'browsertest': ('browser tests', r'_browsertest\.(cc|mm)$', _GTEST_RE),
    'interactive_uitest': (
        'interactive UI tests',
        r'_interactive_ui_?tests?\.(cc|mm)$',
        _GTEST_RE,
    ),
    'webui': ('WebUI tests', r'\.test\.(ts|tsx|js)$', _JS_RE),
}
_EXCLUDE_RE = re.compile(r'(^|/)(third_party|vendor|node_modules)/')

LANES = ('effectiveness', 'best-practices')

# Category slugs -> issue labels.
_FALSE_POSITIVE = {
    'unreached-assertions',
    'no-assertions',
    'tautology',
    'tests-the-mock',
    'production-code-stubbed',
    'name-body-mismatch',
}
TEST_LABELS = {
    'tests/test-quality': (
        'Improving how well tests verify the behavior they claim to cover'
    ),
    'tests/false-positive': (
        'Tests that pass even when the underlying functionality is broken'
    ),
    'tests/weak-assertions': (
        'Tests with insufficient or meaningless assertions'
    ),
    'tests/missing-coverage': (
        "Functionality isn't meaningfully covered despite tests existing"
    ),
    'tests/best-practices': (
        'Tests that break the documented test best practices'
    ),
    'tests/flaky-risk': (
        'Tests whose waits or shared state can make them flaky or '
        'order-dependent'
    ),
}
_BASE_LABELS = ['tests', 'tests/test-quality', 'QA/No', 'release-notes/exclude']
# Path prefix -> existing brave-browser feature label (first match wins).
FEATURE_LABELS = [
    ('components/brave_ads', 'feature/ads'),
    ('browser/brave_ads', 'feature/ads'),
    ('ios/browser/brave_ads', 'feature/ads'),
    ('ios/browser/api/ads', 'feature/ads'),
    ('components/brave_wallet', 'feature/web3/wallet'),
    ('browser/brave_wallet', 'feature/web3/wallet'),
    ('components/brave_news', 'feature/brave-news'),
    ('components/p3a', 'features/P3A'),
    ('components/ntp_background_images', 'feature/new-tab'),
    ('browser/ntp_background', 'feature/new-tab'),
    ('components/playlist', 'features/playlist'),
    ('browser/playlist', 'features/playlist'),
    ('components/psst', 'privacy/psst'),
    ('browser/psst', 'privacy/psst'),
    ('components/brave_sync', 'feature/sync'),
    ('components/sync/', 'feature/sync'),
    ('components/brave_account', 'feature/brave-accounts'),
    ('browser/brave_account', 'feature/brave-accounts'),
    ('components/brave_origin', 'feature/brave-origin'),
    ('browser/brave_origin', 'feature/brave-origin'),
    ('components/brave_vpn', 'feature/vpn'),
    ('browser/brave_vpn', 'feature/vpn'),
    ('components/brave_rewards', 'feature/rewards'),
    ('components/web_discovery', 'feature/web-discovery'),
    ('components/brave_policy', 'policy'),
    ('browser/policy', 'policy'),
    ('components/brave_referrals', 'feature/referral-promo'),
    ('components/sidebar', 'feature/sidebar'),
    ('components/speedreader', 'feature/speedreader'),
    ('browser/workspaces', 'feature/workspaces'),
    ('components/debounce', 'privacy/debounce'),
    ('components/https_upgrade_exceptions', 'privacy/https-upgrades'),
    ('components/brave_shields', 'feature/shields'),
    ('browser/brave_shields', 'feature/shields'),
    ('components/containers', 'feature/containers'),
    ('browser/containers', 'feature/containers'),
    ('components/tor', 'feature/tor'),
    ('browser/importer', 'feature/importer'),
    ('common/importer', 'feature/importer'),
]
_SEV = {'high': 0, 'medium': 1, 'low': 2}

# The best-practices lane audits test files against rules in
# docs/best-practices. Rules are named by their stable anchor id, so they can
# move between docs; the ones below are the test rules that are auditable from
# a test file.
_BP_DIR = _REPO_ROOT / 'docs' / 'best-practices'
# Every rule in these docs must be classified below (audited, reference or
# skipped), so a newly added rule is noticed instead of silently ignored.
_PRACTICE_DOCS = (
    'testing-async.md',
    'testing-isolation.md',
    'testing-javascript.md',
    'testing-navigation.md',
)
_CPP = ('unittest', 'browsertest', 'interactive_uitest')
_BROWSER = ('browsertest', 'interactive_uitest')
_WEBUI = ('webui',)
# (rule id, test types, also judged by the effectiveness lane?, audit note).
# Rules marked True are about assertions that wouldn't fail, which the
# effectiveness lane already judges by mutation; they're dropped when both
# lanes run, so one problem doesn't become two issues.
PRACTICE_RULES = [
    (
        'TA-003',
        _CPP,
        False,
        'Flag sleeps and fixed delays used to wait for something to happen. A '
        'delay inside a deadline-bounded polling loop is the documented '
        'fallback (TJ-002), not a violation.',
    ),
    (
        'TA-005',
        _CPP,
        False,
        'Flag every `RunUntilIdle()` use. Group all occurrences in the file '
        'into one finding.',
    ),
    (
        'TA-008',
        _BROWSER,
        False,
        '`EvalJs`/`ExecJs` or another blocking IPC inside a `RunUntil` '
        'lambda.',
    ),
    (
        'TA-009',
        _CPP,
        False,
        '`RunLoop` + `BindLambdaForTesting` + `Quit()` around an API whose '
        'callback a `TestFuture` could capture. Not for observers, for '
        'callbacks that fire several times, or for timeout/cancellation '
        'tests. Group all occurrences in the file into one finding.',
    ),
    (
        'TA-012',
        _BROWSER,
        False,
        '`EvalJs` right after an async mojo call whose effect it checks, with '
        'no wait in between.',
    ),
    (
        'TI-002',
        _CPP,
        False,
        'Assertions on private state or internals instead of public behavior. '
        'A single `*ForTesting()` accessor is accepted (TI-011, ARCH-020).',
    ),
    (
        'TI-003',
        _BROWSER,
        False,
        'One expected value shared by requests to different domains or '
        'subresources.',
    ),
    (
        'TI-004',
        _BROWSER,
        False,
        'Throttle or delay tests whose window is not much larger than the '
        'time the test spends polling.',
    ),
    (
        'TI-010',
        ('unittest',),
        False,
        'Tests under `components/` that include `chrome/browser/...` or '
        'depend on `//chrome` (check includes and the BUILD.gn deps).',
    ),
    (
        'TI-011',
        _CPP,
        False,
        'Several `FRIEND_TEST_ALL_PREFIXES` for one member that one '
        '`*_for_testing()` accessor would serve.',
    ),
    (
        'TI-013',
        _CPP,
        False,
        'A test-only global or flag changed in `SetUp` and never restored '
        '(`TearDown` or a scoped RAII helper counts as restoring).',
    ),
    (
        'TI-020',
        _CPP,
        False,
        'JSON checked with `HasSubstr` or string comparison, or an expected '
        'dict built by hand where a `ParseJsonDict` literal would do.',
    ),
    (
        'TI-025',
        _CPP,
        False,
        '`value_or(...)` fallbacks in assertions, and optional access with no '
        'earlier `ASSERT_TRUE(has_value())`.',
    ),
    (
        'TI-027',
        _CPP,
        False,
        'Setup that writes internal state straight into a configuration '
        'production code cannot reach. Check the production callers first.',
    ),
    (
        'TI-028',
        _CPP,
        False,
        '`WillRepeatedly` on an `EXPECT_CALL` for a method that should run '
        'exactly once. Not for stubs whose call count is irrelevant '
        '(`ON_CALL`, getters, `NiceMock` defaults).',
    ),
    (
        'TI-034',
        _CPP,
        False,
        '`EXPECT_*` guarding a pointer, optional, size or index that the next '
        'lines dereference or index.',
    ),
    (
        'TI-037',
        _CPP,
        False,
        'Test HTTP handlers and interceptors whose default branch silently '
        'returns null, empty or 404.',
    ),
    (
        'TI-042',
        _CPP,
        False,
        'A `base::Feature` whose default the test relies on without pinning '
        'it, or a `ScopedFeatureList` initialized after the objects that read '
        'the flag are constructed.',
    ),
    (
        'TJ-004',
        _BROWSER,
        False,
        'Using a JS API after waiting only for its mojo binding.',
    ),
    (
        'TN-001',
        _BROWSER,
        False,
        'Polling or `TestNavigationObserver` waiting on a same-document '
        '(hash) navigation.',
    ),
    (
        'TN-002',
        _BROWSER,
        False,
        '`setTimeout` delays in evaluated JS used to wait for a condition.',
    ),
    (
        'TN-003',
        _BROWSER,
        False,
        'Speedreader tests that interact with distilled content after '
        '`NavigateToPageSynchronously` without `WaitDistilled`.',
    ),
    (
        'CS-061',
        _CPP,
        False,
        '`EXPECT_DEATH`/`ASSERT_DEATH` (or CHECK death variants) in a suite '
        'whose name does not end in `DeathTest`.',
    ),
    (
        'CSA-051',
        _CPP,
        False,
        'Hand-rolled `JSONReader::Read` + `GetDict()`/`FindString` chains in '
        'assertions where `base::test::ParseJson*`/`DictHasValue` apply. Cite '
        'TI-020 instead when the problem is substring matching.',
    ),
    (
        'ARCH-062',
        ('unittest',),
        False,
        'Only the "do not use `BrowserWithTestWindowTest`" part. Ignore the '
        'CUJ test requirement.',
    ),
    (
        'BS-033',
        ('unittest',),
        False,
        'A unit test under `browser/` whose code under test lives in '
        '`components/`.',
    ),
    (
        'BS-023',
        _BROWSER,
        False,
        'A browser test of code that also ships on Android deriving from '
        '`InProcessBrowserTest` instead of `PlatformBrowserTest`. Skip '
        'desktop-only features.',
    ),
    (
        'FE-016',
        _WEBUI,
        False,
        '`waitFor` around updates that `rerender`/`act` already apply '
        'synchronously.',
    ),
    # Judged by the effectiveness lane too.
    ('TI-008', _BROWSER, True, '`EvalJs`/`ExecJs` whose result is discarded.'),
    (
        'TI-012',
        _CPP,
        True,
        'A test-only switch or override that turns off the code the test name '
        'says it verifies.',
    ),
    (
        'TI-016',
        ('unittest',),
        True,
        'A tool, handler or parser entry point that takes external input and '
        'is tested only with valid input.',
    ),
    ('TI-018', _CPP, True, 'Ordered results asserted by content only.'),
    (
        'TI-021',
        _CPP,
        True,
        'Error-path tests that do not check the side effects that must not '
        'happen.',
    ),
    (
        'TI-022',
        ('unittest', 'browsertest'),
        True,
        'Serialization tests that skip fields or invalid input.',
    ),
    ('TI-026', _CPP, True, 'A guard condition tested on one side only.'),
    (
        'TI-029',
        _BROWSER,
        True,
        'A browser test asserting a return value but not the observable '
        'effect.',
    ),
    (
        'ARCH-062b',
        _CPP,
        True,
        'A test whose logic mirrors the production logic and breaks on any '
        'refactor without catching a bug.',
    ),
    (
        'CSRC-002',
        _CPP,
        True,
        '`#define X DISABLED_X` in a chromium_src test override.',
    ),
    (
        'FE-027',
        _WEBUI,
        True,
        '`expect(render(...)).toBeTruthy()` and similar always-true checks.',
    ),
]
# Rules whose violation risks flakiness or order dependence; findings under
# them also get the `tests/flaky-risk` label.
_FLAKY_RULES = {
    'TA-003',
    'TA-005',
    'TA-008',
    'TA-012',
    'TI-003',
    'TI-004',
    'TI-013',
    'TI-042',
    'TJ-004',
    'TN-001',
    'TN-002',
    'TN-003',
}
# Preferred patterns. Shown so a finding's suggested_fix can point at them;
# never cited as a violation.
PRACTICE_REFS = [
    ('TA-006', _CPP),
    ('TA-010', _BROWSER),
    ('TJ-001', _BROWSER),
    ('TJ-002', _BROWSER),
]
# Rules in _PRACTICE_DOCS that can't be audited from a test file, with why.
PRACTICE_SKIP = {
    'TA-001': 'section header',
    'TA-002': 'how to fix a flake, not a property of a test',
    'TA-004': 'rule of thumb for fixes',
    'TA-011': 'an alternative, not a rule',
    'TI-001': 'subjective: when a real dependency is acceptable',
    'TI-005': 'section header',
    'TI-006': 'research process',
    'TI-007': 'comment convention',
    'TI-009': 'maintainability, not correctness',
    'TI-014': 'style: assertion messages',
    'TI-015': 'style: SCOPED_TRACE in helpers',
    'TI-017': 'refactoring process',
    'TI-023': 'style: parameterize similar tests',
    'TI-024': 'style: extract assertion helpers',
    'TI-030': 'PR process',
    'TI-033': 'PR process',
    'TI-035': 'production code, not the test',
    'TI-036': 'style: TEST vs TEST_F',
    'TI-038': 'style: test names',
    'TI-039': 'PR process and coverage',
    'TI-040': 'coverage of chromium_src overrides',
    'TI-041': 'coverage: flag combinations',
    'TI-043': 'how to run tests',
    'TJ-003': 'tests of page-visible APIs deliberately run in the main world',
}


def _run(cmd, env=None, cwd=None):
    return subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        env=env,
        cwd=cwd,
        check=False,
        timeout=180,
    )


def _read_json(path):
    with open(path, encoding='utf-8') as f:
        return json.load(f)


def _write_json(path, data):
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=1)


def _read_jsonl(path):
    rows, bad = [], 0
    if not path.exists():
        return rows, bad
    with open(path, encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rows.append(json.loads(line))
            except ValueError:
                bad += 1
    return rows, bad


def _append_jsonl(path, row):
    with open(path, 'a', encoding='utf-8') as f:
        f.write(json.dumps(row) + '\n')


def _shard(items, max_lines, max_items=None):
    """Greedy split of (key, lines, weight) items, keeping order."""
    shards, cur, cur_lines, cur_items = [], [], 0, 0
    for key, lines, weight in items:
        over = cur and (
            cur_lines + lines > max_lines
            or (max_items and cur_items + weight > max_items)
        )
        if over:
            shards.append(cur)
            cur, cur_lines, cur_items = [], 0, 0
        cur.append((key, lines, weight))
        cur_lines += lines
        cur_items += weight
    if cur:
        shards.append(cur)
    return shards


# lane -> (template, name in the work dir); same names so agent prompts match.
_BRIEFS = {
    'effectiveness': (
        ('audit_brief.md', 'AUDIT_BRIEF.md'),
        ('verify_brief.md', 'VERIFY_BRIEF.md'),
        ('bug_brief.md', 'BUG_BRIEF.md'),
    ),
    'best-practices': (
        ('bp_audit_brief.md', 'AUDIT_BRIEF.md'),
        ('bp_verify_brief.md', 'VERIFY_BRIEF.md'),
        ('bug_brief.md', 'BUG_BRIEF.md'),
    ),
}


def _fill(template, meta):
    text = (_SKILL_DIR / template).read_text(encoding='utf-8')
    for key in ('DIR', 'TYPE', 'TYPE_NAME', 'REPO_ROOT'):
        text = text.replace('{%s}' % key, str(meta[key.lower()]))
    return text


def _load_rules():
    """rule id -> {doc, title, text} for every anchored rule in the docs."""
    rules = {}
    for doc in sorted(_BP_DIR.glob('*.md')):
        text = doc.read_text(encoding='utf-8')
        for part in re.split(r'(?=<a id="[A-Za-z0-9-]+"></a>)', text):
            m = re.match(r'<a id="([A-Za-z0-9-]+)"></a>', part)
            if not m:
                continue
            body = re.sub(r'<a id="[^"]*"></a>\s*', '', part).strip()
            body = re.sub(r'\n-{3,}\s*$', '', body).strip()
            head = re.match(r'#+\s*(.+)\n?', body)
            title = re.sub(r'^[✅❌\s]+', '', head.group(1)) if head else ''
            rules[m.group(1)] = {
                'doc': doc.name,
                'title': title or m.group(1),
                'text': body[head.end():].strip() if head else body,
            }
    return rules


def _practices(test_type, skip_overlap):
    """Rules for a test type -> (PRACTICES.md text, {id: {doc, title}})."""
    rules = _load_rules()
    audit = [
        r
        for r in PRACTICE_RULES
        if test_type in r[1] and not (skip_overlap and r[2])
    ]
    refs = [i for i, types in PRACTICE_REFS if test_type in types]
    known = (
        {r[0] for r in PRACTICE_RULES}
        | {i for i, _ in PRACTICE_REFS}
        | set(PRACTICE_SKIP)
    )
    missing = sorted(known - set(rules))
    if missing:
        sys.exit(
            'rules not found in docs/best-practices: %s (renamed or removed? '
            'update the tables in tq.py)' % ', '.join(missing)
        )
    unclassified = sorted(
        i
        for i, r in rules.items()
        if r['doc'] in _PRACTICE_DOCS and i not in known
    )
    if unclassified:
        print(
            'warning: %d rule(s) in the testing docs are not classified in '
            'tq.py and will not be audited: %s'
            % (len(unclassified), ', '.join(unclassified))
        )
    if not audit:
        sys.exit('no best-practice rules apply to %s' % test_type)

    def section(rid, note=None):
        r = rules[rid]
        out = ['### %s: %s (%s)' % (rid, r['title'], r['doc']), '']
        if note:
            out += ['**Audit note:** ' + note, '']
        return out + [r['text'], '']

    lines = [
        '# Best practices to audit (%s)' % TYPES[test_type][0],
        '',
        'Rule text is copied from `docs/best-practices/`. Rule ids are stable.',
        '',
        '## Rules to audit',
        '',
    ]
    for rid, _, _, note in audit:
        lines += section(rid, note)
    if refs:
        lines += [
            '## Preferred patterns',
            '',
            'Use these to write `suggested_fix`. Never report them as '
            'violations.',
            '',
        ]
        for rid in refs:
            lines += section(rid)
    used = [r[0] for r in audit] + refs
    return '\n'.join(lines), {
        i: {'doc': rules[i]['doc'], 'title': rules[i]['title']} for i in used
    }


def cmd_init(args):
    type_name, file_re, test_re = TYPES[args.type]
    root = Path(
        args.out
        or '/tmp/brave-test-quality/%s-%s'
        % (args.type, datetime.datetime.now().strftime('%Y%m%d-%H%M'))
    )
    files = _run(['git', 'ls-files'], cwd=_REPO_ROOT).stdout.split()
    files = sorted(
        f
        for f in files
        if re.search(file_re, f)
        and not _EXCLUDE_RE.search(f)
        and (not args.path or any(f.startswith(p) for p in args.path))
    )
    if not files:
        sys.exit('no %s files matched' % args.type)
    test_rx = re.compile(test_re)
    rows = []
    for f in files:
        text = (_REPO_ROOT / f).read_text(encoding='utf-8', errors='replace')
        lines = text.splitlines()
        rows.append(
            (f, len(lines), sum(1 for line in lines if test_rx.match(line)))
        )
    sha = _run(['git', 'rev-parse', 'HEAD'], cwd=_REPO_ROOT).stdout.strip()
    shards = _shard(rows, args.max_lines)
    lanes = LANES if args.lane == 'all' else (args.lane,)
    for lane in lanes:
        meta = {
            'dir': str(root / lane),
            'lane': lane,
            'type': args.type,
            'type_name': type_name,
            'repo_root': str(_REPO_ROOT),
            'sha': sha,
            'paths': args.path or [],
        }
        practices = None
        if lane == 'best-practices':
            # With both lanes running, leave assertion-strength rules to the
            # effectiveness lane.
            practices, meta['rules'] = _practices(
                args.type, skip_overlap=args.lane == 'all'
            )
        _init_lane(root / lane, meta, rows, shards, practices)
        print(
            '%s: %s | %d files, %d lines, %d tests, %d audit shards%s'
            % (
                lane,
                root / lane,
                len(rows),
                sum(r[1] for r in rows),
                sum(r[2] for r in rows),
                len(shards),
                ', %d rules' % len(meta['rules']) if practices else '',
            )
        )


def _init_lane(out, meta, rows, shards, practices):
    for sub in (
        'audit/shards',
        'audit/results',
        'verify/work',
        'verify/results',
        'verify/scratch',
        'bugs',
        'final',
        'post',
    ):
        (out / sub).mkdir(parents=True, exist_ok=True)
    _write_json(out / 'meta.json', meta)
    with open(out / 'manifest.tsv', 'w', encoding='utf-8') as f:
        f.write('path\tlines\ttests\n')
        for r in rows:
            f.write('%s\t%d\t%d\n' % r)
    for i, shard in enumerate(shards, 1):
        with open(
            out / ('audit/shards/shard-%02d.txt' % i), 'w', encoding='utf-8'
        ) as f:
            for p, nlines, ntests in shard:
                f.write('%s\t%d lines\t%d tests\n' % (p, nlines, ntests))
    for src, dst in _BRIEFS[meta['lane']]:
        (out / dst).write_text(_fill(src, meta), encoding='utf-8')
    if practices:
        (out / 'PRACTICES.md').write_text(practices, encoding='utf-8')


def _audit_rows(d):
    rows = {}
    for fn in sorted((d / 'audit/results').glob('shard-*.jsonl')):
        for r in _read_jsonl(fn)[0]:
            rows[r['path']] = r  # last write wins
    return rows


def _verify_rows(d):
    rows = {}
    for fn in sorted((d / 'verify/results').glob('shard-*.jsonl')):
        for r in _read_jsonl(fn)[0]:
            rows[r['id']] = r
    return rows


def _lane(d):
    # Work dirs made before lanes existed have no `lane`.
    return _read_json(d / 'meta.json').get('lane', 'effectiveness')


def _flagged(row):
    # Tests with a finding: `weak` (effectiveness lane) or `flagged`.
    return row.get('flagged', row.get('weak', 0))


def _lane_dirs(path):
    """A lane dir, or the lane dirs under an output dir."""
    path = Path(path)
    if not path.is_dir():
        sys.exit('no such dir: %s' % path)
    if (path / 'meta.json').exists():
        return [path]
    dirs = [p for p in sorted(path.iterdir()) if (p / 'meta.json').exists()]
    if not dirs:
        sys.exit('no work dir at %s' % path)
    return dirs


def cmd_status(args):
    d = Path(args.dir)
    out = []
    shards = sorted((d / 'audit/shards').glob('shard-*.txt'))
    done = 0
    for s in shards:
        want = len(s.read_text(encoding='utf-8').splitlines())
        got = len(_read_jsonl(d / 'audit/results' / (s.stem + '.jsonl'))[0])
        done += got >= want
    files = _audit_rows(d)
    flagged = sum(_flagged(r) for r in files.values())
    out.append(
        'audit: %d/%d shards, %d files, %d %s tests'
        % (
            done,
            len(shards),
            len(files),
            flagged,
            'weak' if _lane(d) == 'effectiveness' else 'flagged',
        )
    )
    work = sorted((d / 'verify/work').glob('shard-*.json'))
    if work:
        want = sum(len(x['findings']) for w in work for x in _read_json(w))
        res = _verify_rows(d)
        v = collections.Counter(r.get('verdict') for r in res.values())
        out.append(
            'verify: %d/%d findings | %s'
            % (
                len(res),
                want,
                ' '.join('%s=%d' % kv for kv in sorted(v.items())),
            )
        )
    posted = _read_jsonl(d / 'post/posted.jsonl')[0]
    if posted:
        out.append('posted: %d' % len(posted))
    print(' | '.join(out), flush=True)


def cmd_verify_work(args):
    d = Path(args.dir)
    manifest = {}
    with open(d / 'manifest.tsv', encoding='utf-8') as f:
        next(f)
        for line in f:
            p, n, _ = line.rstrip('\n').split('\t')
            manifest[p] = int(n)
    items = []
    lane = _lane(d)
    for p, r in sorted(_audit_rows(d).items()):
        findings = []
        for i, f in enumerate(r.get('findings', [])):
            rec = {
                'id': '%s#%d' % (p, i),
                'tests': f.get('tests', []),
                'line': f.get('line'),
                'confidence': f.get('confidence'),
                'claim': f.get('summary', ''),
                'suggested_fix': f.get('suggested_fix', ''),
            }
            if lane == 'best-practices':
                rec['rule'] = f.get('rule')
                rec['evidence'] = f.get('evidence', '')
            else:
                rec['category'] = f.get('category')
                rec['mutation'] = f.get('regression', '')
                rec['production_ref'] = f.get('production_ref', '')
            findings.append(rec)
        if findings:
            items.append((p, manifest.get(p, 0), findings))
    shards = _shard(
        [(p, nlines, len(fs)) for p, nlines, fs in items],
        args.max_lines,
        args.max_findings,
    )
    by_path = {p: fs for p, _, fs in items}
    for old in (d / 'verify/work').glob('shard-*.json'):
        old.unlink()
    for i, shard in enumerate(shards, 1):
        _write_json(
            d / ('verify/work/shard-%02d.json' % i),
            [
                {'path': p, 'lines': nlines, 'findings': by_path[p]}
                for p, nlines, _ in shard
            ],
        )
    print(
        '%d findings in %d files -> %d verify shards'
        % (sum(len(fs) for _, _, fs in items), len(items), len(shards))
    )


def cmd_bug_work(args):
    d = Path(args.dir)
    cands = []
    for r in _verify_rows(d).values():
        bug = r.get('production_bug')
        if bug and str(bug).strip().lower() not in ('null', 'none', ''):
            cands.append({'from_finding': r['id'], 'claim': str(bug)})
    _write_json(d / 'bugs/candidates.json', cands)
    print(
        '%d production-bug candidates -> %s'
        % (len(cands), d / 'bugs/candidates.json')
    )


def _link(blob, ref):
    m = re.match(r'^\s*([\w./+-]+?)(?::(\d+)(?:-(\d+))?)?\s*$', ref or '')
    if not m or not (_REPO_ROOT / m.group(1)).is_file():
        return '`%s`' % ref
    url = blob + m.group(1)
    if m.group(2):
        url += '#L%s' % m.group(2) + ('-L%s' % m.group(3) if m.group(3) else '')
    return '[`%s`](%s)' % (ref.strip(), url)


def _clean(text):
    # Don't notify GitHub users/teams, and keep cross-repo refs pointing at
    # brave-core (issues are filed in brave-browser).
    text = re.sub(r'(?<![`\w])@(?=[A-Za-z0-9_-])', '@\u200b', text or '')
    return re.sub(r'(?<![\w/])#(\d{4,})', r'brave/brave-core#\1', text)


def _feature_label(path):
    for prefix, label in FEATURE_LABELS:
        if path.startswith(prefix):
            return label
    return None


def _sev(verdict):
    sev = verdict.get('severity')
    return sev if sev in _SEV else 'low'


def _by_file(verdicts, work, min_sev):
    """path -> ids of kept findings (CONFIRMED/NARROWED), worst first."""
    files = collections.defaultdict(list)
    for i, v in verdicts.items():
        if (
            v.get('verdict') in ('CONFIRMED', 'NARROWED')
            and i in work
            and _SEV[_sev(v)] <= _SEV[min_sev]
        ):
            files[work[i][0]].append(i)
    for ids in files.values():
        ids.sort(
            key=lambda i: (
                _SEV[_sev(verdicts[i])],
                work[i][1].get('line') or 0,
            )
        )
    return files


def _path_labels(path):
    labels = []
    feature = _feature_label(path)
    if feature:
        labels.append(feature)
    if path.startswith('ios/'):
        labels.append('OS/iOS')
    return labels


def _short_name(path, names):
    name = os.path.basename(path)
    return '/'.join(path.split('/')[-2:]) if names[name] > 1 else name


def _test_issues(meta, verdicts, work, min_sev):
    blob = 'https://github.com/brave/brave-core/blob/%s/' % meta['sha']
    files = _by_file(verdicts, work, min_sev)
    names = collections.Counter(os.path.basename(p) for p in files)
    issues = []
    for p, ids in sorted(files.items()):
        top = _sev(verdicts[ids[0]])
        body = [
            '%s in [`%s`](%s) would keep passing if the behavior they cover '
            'broke.' % (meta['type_name'].capitalize(), p, blob + p),
            '',
        ]
        cats = set()
        for i in ids:
            v, f = verdicts[i], work[i][1]
            cats.add(v.get('category') or f.get('category'))
            tests = v.get('weak_tests') or f.get('tests') or []
            line = (
                ' [L%s](%s#L%s)' % (f['line'], blob + p, f['line'])
                if f.get('line')
                else ''
            )
            body += [
                '### %s (%s severity)%s'
                % (
                    ', '.join('`%s`' % t for t in tests),
                    _sev(v),
                    line,
                ),
                '',
                "- **Why it's weak:** %s"
                % _clean(v.get('issue_text') or f.get('claim')),
                '- **Change that would still pass:** %s'
                % _clean(v.get('mutation') or f.get('mutation')),
                '- **Production code:** %s'
                % _link(
                    blob, v.get('production_ref') or f.get('production_ref')
                ),
                '- **Suggested fix:** %s'
                % _clean(v.get('suggested_fix') or f.get('suggested_fix')),
                '',
            ]
        body.append(
            '_Found by an AI-assisted audit asking "if this functionality '
            'broke, would the test fail?" Read-only analysis (no mutation was '
            'run); each item was independently re-verified against the '
            'production code and the rest of the repo\'s tests. Line links are '
            'pinned to %s._' % meta['sha'][:11]
        )
        labels = list(_BASE_LABELS)
        if cats & _FALSE_POSITIVE:
            labels.append('tests/false-positive')
        if 'loose-assertions' in cats:
            labels.append('tests/weak-assertions')
        if 'missing-key-case' in cats:
            labels.append('tests/missing-coverage')
        labels += _path_labels(p)
        issues.append(
            {
                'key': p,
                'title': 'Weak %s in %s'
                % (meta['type_name'], _short_name(p, names)),
                'body': '\n'.join(body) + '\n',
                'labels': labels,
                'severity': top,
                'findings': len(ids),
            }
        )
    issues.sort(key=lambda x: (_SEV[x['severity']], x['key']))
    return issues


def _bp_issues(meta, verdicts, work, min_sev):
    sha = meta['sha']
    blob = 'https://github.com/brave/brave-core/blob/%s/' % sha
    rules = meta.get('rules', {})
    files = _by_file(verdicts, work, min_sev)
    names = collections.Counter(os.path.basename(p) for p in files)
    issues = []
    for p, ids in sorted(files.items()):
        body = [
            '%s in [`%s`](%s) break test best practices documented in '
            '`docs/best-practices`.'
            % (meta['type_name'].capitalize(), p, blob + p),
            '',
        ]
        rule_ids = set()
        for i in ids:
            v, f = verdicts[i], work[i][1]
            rid = v.get('rule') or f.get('rule') or ''
            rule_ids.add(rid)
            rule = rules.get(rid)
            rule_text = '`%s`' % rid
            if rule:
                rule_text = '[%s: %s](%sdocs/best-practices/%s#%s)' % (
                    rid,
                    rule['title'],
                    blob,
                    rule['doc'],
                    rid,
                )
            tests = v.get('violating_tests') or f.get('tests') or []
            line = (
                ' [L%s](%s#L%s)' % (f['line'], blob + p, f['line'])
                if f.get('line')
                else ''
            )
            body += [
                '### %s (%s severity)%s'
                % (
                    ', '.join('`%s`' % t for t in tests) or 'File-wide',
                    _sev(v),
                    line,
                ),
                '',
                '- **Rule:** %s' % rule_text,
                '- **Problem:** %s'
                % _clean(v.get('issue_text') or f.get('claim')),
            ]
            evidence = (v.get('evidence') or f.get('evidence') or '').strip()
            if evidence:
                body.append('- **Code:** `%s`' % evidence.replace('`', "'"))
            body += [
                '- **Suggested fix:** %s'
                % _clean(v.get('suggested_fix') or f.get('suggested_fix')),
                '',
            ]
        body.append(
            "_Found by an AI-assisted audit of this file against Brave's test "
            'best practices; each item was independently re-verified against '
            'the rule text and the surrounding code. Line links are pinned to '
            '%s._' % sha[:11]
        )
        issues.append(
            {
                'key': p,
                'title': 'Test best-practice violations in %s'
                % _short_name(p, names),
                'body': '\n'.join(body) + '\n',
                'labels': list(_BASE_LABELS)
                + ['tests/best-practices']
                + (['tests/flaky-risk'] if rule_ids & _FLAKY_RULES else [])
                + _path_labels(p),
                'severity': _sev(verdicts[ids[0]]),
                'findings': len(ids),
            }
        )
    issues.sort(key=lambda x: (_SEV[x['severity']], x['key']))
    return issues


def _bug_issues(d, meta):
    blob = 'https://github.com/brave/brave-core/blob/%s/' % meta['sha']
    rows = _read_jsonl(d / 'bugs/verified.jsonl')[0]
    bugs, seen = [], set()
    for r in rows:
        if r.get('verdict') not in ('REAL', 'LATENT') or r['title'] in seen:
            continue
        seen.add(r['title'])
        loc = r.get('location', '')
        body = '%s\n\n%s\n\n**When it happens:** %s\n\n**Fix:** %s' % (
            _link(blob, loc),
            _clean(r.get('explanation')),
            _clean(r.get('reachability')),
            _clean(r.get('fix')),
        )
        if r['verdict'] == 'LATENT':
            body += (
                '\n\n_Latent: not reachable in production today, but the '
                'code is wrong._'
            )
        body += (
            '\n\n_Found by an AI-assisted code audit and confirmed by '
            'reading the code (not reproduced at runtime). Line links are '
            'pinned to %s._\n' % meta['sha'][:11]
        )
        labels = ['bug']
        # `CHECK` is case-sensitive so the word "check" doesn't match.
        if r['verdict'] == 'REAL' and re.search(
            r'(?i:crash)|\bCHECK', r['title'] + r.get('explanation', '')
        ):
            labels.append('crash')
        feature = _feature_label(loc)
        if feature:
            labels.append(feature)
        if loc.startswith('ios/'):
            labels.append('OS/iOS')
        bugs.append(
            {
                'key': r['title'],
                'title': r['title'].replace('`', ''),
                'body': body,
                'labels': labels,
                'severity': r.get('severity') or 'low',
                'status': r['verdict'],
            }
        )
    bugs.sort(
        key=lambda b: (
            b['status'] != 'REAL',
            _SEV.get(b['severity'], 3),
            b['key'],
        )
    )
    return bugs


def cmd_aggregate(args):
    d = Path(args.dir)
    meta = _read_json(d / 'meta.json')
    lane = meta.get('lane', 'effectiveness')
    work = {}
    for w in sorted((d / 'verify/work').glob('shard-*.json')):
        for item in _read_json(w):
            for f in item['findings']:
                work[f['id']] = (item['path'], f)
    verdicts = _verify_rows(d)
    missing = sorted(set(work) - set(verdicts))
    audit = _audit_rows(d)
    print(
        'audit: %d files, %d tests reviewed, %d %s, %d findings'
        % (
            len(audit),
            sum(r.get('tests_reviewed', 0) for r in audit.values()),
            sum(_flagged(r) for r in audit.values()),
            'weak' if lane == 'effectiveness' else 'flagged',
            len(work),
        )
    )
    v = collections.Counter(
        verdicts[i].get('verdict') for i in work if i in verdicts
    )
    print('verify: %s | missing %d' % (dict(v), len(missing)))
    for m in missing[:10]:
        print('  missing:', m)

    build = _test_issues if lane == 'effectiveness' else _bp_issues
    issues = build(meta, verdicts, work, args.min_severity)
    drafts = d / 'final/drafts'
    drafts.mkdir(parents=True, exist_ok=True)
    for old in drafts.glob('*.md'):
        old.unlink()
    for x in issues:
        slug = re.sub(r'[^A-Za-z0-9_.-]', '_', x['key'].replace('/', '__'))
        (drafts / ('%s__%s.md' % (x['severity'], slug))).write_text(
            '# %s\n\nlabels: %s\n\n%s'
            % (x['title'], ', '.join(x['labels']), x['body']),
            encoding='utf-8',
        )
    _write_json(d / 'final/issues.json', issues)
    sev = collections.Counter(x['severity'] for x in issues)
    print(
        'issues: %d (by top severity %s) -> %s'
        % (len(issues), dict(sev), d / 'final/issues.json')
    )

    if (d / 'bugs/verified.jsonl').exists():
        bugs = _bug_issues(d, meta)
        _write_json(d / 'final/bug_issues.json', bugs)
        with open(d / 'final/bug_issues.md', 'w', encoding='utf-8') as f:
            for b in bugs:
                f.write(
                    '# [%s/%s] %s\nlabels: %s\n\n%s\n---\n\n'
                    % (
                        b['status'],
                        b['severity'],
                        b['title'],
                        ', '.join(b['labels']),
                        b['body'],
                    )
                )
        print('bug issues: %d -> %s' % (len(bugs), d / 'final/bug_issues.md'))


def cmd_labels(args):
    for name, desc in TEST_LABELS.items():
        r = _run(
            [
                'gh',
                'label',
                'create',
                name,
                '-R',
                args.repo,
                '--color',
                'd4c5f9',
                '--description',
                desc,
            ]
        )
        print(
            '%s: %s'
            % (
                name,
                'created'
                if r.returncode == 0
                else (r.stderr.strip() or 'exists')[:120],
            )
        )


def _log(d, msg):
    line = '%s %s' % (time.strftime('%Y-%m-%d %H:%M:%S'), msg)
    print(line, flush=True)
    with open(d / 'post/post.log', 'a', encoding='utf-8') as f:
        f.write(line + '\n')


def _existing(repo, account, label, env):
    r = _run(
        [
            'gh',
            'issue',
            'list',
            '-R',
            repo,
            '--author',
            account,
            '--label',
            label,
            '--state',
            'all',
            '--limit',
            '5000',
            '--json',
            'title,url',
        ],
        env=env,
    )
    if r.returncode != 0:
        sys.exit('could not list existing issues: ' + r.stderr[:300])
    return {x['title']: x['url'] for x in json.loads(r.stdout)}


def _create(issue, repo, env):
    with tempfile.NamedTemporaryFile(
        'w', suffix='.md', delete=False, encoding='utf-8'
    ) as f:
        f.write(issue['body'])
        body_file = f.name
    cmd = [
        'gh',
        'issue',
        'create',
        '-R',
        repo,
        '--title',
        issue['title'],
        '--body-file',
        body_file,
    ]
    for label in issue['labels']:
        cmd += ['--label', label]
    try:
        return _run(cmd, env=env)
    finally:
        os.unlink(body_file)


def cmd_post(args):
    d = Path(args.dir)
    token = _run(['gh', 'auth', 'token', '--user', args.account]).stdout.strip()
    if not token:
        sys.exit(
            'no gh token for %s (gh auth login, then retry)' % args.account
        )
    env = dict(os.environ, GH_TOKEN=token)
    login = _run(['gh', 'api', 'user', '--jq', '.login'], env=env).stdout
    if login.strip() != args.account:
        sys.exit('token is for %r, expected %r' % (login.strip(), args.account))
    if args.bugs:
        issues, label = _read_json(d / 'final/bug_issues.json'), 'bug'
    else:
        issues = _read_json(d / 'final/issues.json')
        label = (
            'tests/best-practices'
            if _lane(d) == 'best-practices'
            else 'tests/test-quality'
        )
    if args.one:
        issues = [x for x in issues if x['key'] == args.one]
        if not issues:
            sys.exit('no such key: ' + args.one)
    want = {label for x in issues for label in x['labels']}
    have = _run(
        [
            'gh',
            'label',
            'list',
            '-R',
            args.repo,
            '--limit',
            '2000',
            '--json',
            'name',
            '--jq',
            '.[].name',
        ],
        env=env,
    ).stdout.split('\n')
    if want - set(have):
        sys.exit(
            'missing labels in %s: %s (tq.py labels creates the tests/* '
            'ones; needs write access)' % (args.repo, sorted(want - set(have)))
        )
    gap = tuple(int(x) for x in args.gap.split(','))
    stop = d / 'post/STOP'
    done = _existing(args.repo, args.account, label, env)
    todo = [x for x in issues if x['title'] not in done]
    _log(
        d,
        'start: %d to post, %d already on GitHub'
        % (len(todo), len(issues) - len(todo)),
    )
    failures = 0
    for n, x in enumerate(todo, 1):
        if stop.exists():
            _log(d, 'STOP found; %d left' % (len(todo) - n + 1))
            return
        ok = False
        attempts = 4
        for attempt in range(attempts):
            if attempt:
                # A failed create can still have created the issue.
                url = _existing(args.repo, args.account, label, env).get(
                    x['title']
                )
                if url:
                    ok = True
                    break
            r = _create(x, args.repo, env)
            url = r.stdout.strip()
            if r.returncode == 0 and url.startswith('https://'):
                ok = True
                break
            err = (r.stderr + r.stdout)[:300]
            if attempt == attempts - 1:
                _log(d, 'FAILED %s: %s; giving up' % (x['key'], err))
                break
            rate = re.search(r'rate limit|secondary|abuse', err, re.I)
            wait = (
                random.randint(1800, 3600) if rate else random.randint(120, 300)
            )
            _log(d, 'FAILED %s: %s; retry in %ds' % (x['key'], err, wait))
            time.sleep(wait)
        if ok:
            failures = 0
            _append_jsonl(
                d / 'post/posted.jsonl',
                {'key': x['key'], 'title': x['title'], 'url': url},
            )
            _log(d, 'posted %s -> %s' % (x['key'], url))
        else:
            failures += 1
            if failures >= 3:
                _log(d, '3 consecutive failures; stopping')
                return
        if n < len(todo):
            time.sleep(random.randint(*gap))
    _log(d, 'done')


def main():
    p = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter
    )
    sub = p.add_subparsers(dest='cmd', required=True)
    s = sub.add_parser('init')
    s.add_argument('--type', required=True, choices=sorted(TYPES))
    s.add_argument('--path', action='append', help='limit to a path prefix')
    s.add_argument('--out', help='output dir (default under /tmp)')
    s.add_argument(
        '--lane',
        default='all',
        choices=LANES + ('all',),
        help='what to check (default: all lanes, one work dir each)',
    )
    s.add_argument('--max-lines', type=int, default=9500)
    for name in ('status', 'bug-work'):
        sub.add_parser(name).add_argument('dir')
    s = sub.add_parser('aggregate')
    s.add_argument('dir')
    s.add_argument(
        '--min-severity',
        default='low',
        choices=sorted(_SEV, key=_SEV.get),
        help='drop findings below this severity from the issues',
    )
    s = sub.add_parser('verify-work')
    s.add_argument('dir')
    s.add_argument('--max-findings', type=int, default=20)
    s.add_argument('--max-lines', type=int, default=7000)
    s = sub.add_parser('labels')
    s.add_argument('--repo', default='brave/brave-browser')
    s = sub.add_parser('post')
    s.add_argument('dir')
    s.add_argument('--account', required=True)
    s.add_argument('--repo', default='brave/brave-browser')
    s.add_argument('--gap', default='15,35', help='seconds between posts')
    g = s.add_mutually_exclusive_group(required=True)
    g.add_argument('--one', metavar='KEY', help='post one issue (by key)')
    g.add_argument('--batch', action='store_true', help='post test issues')
    g.add_argument('--bugs', action='store_true', help='post bug issues')
    args = p.parse_args()
    cmds = {
        'init': cmd_init,
        'status': cmd_status,
        'verify-work': cmd_verify_work,
        'bug-work': cmd_bug_work,
        'aggregate': cmd_aggregate,
        'labels': cmd_labels,
        'post': cmd_post,
    }
    if args.cmd in ('status', 'verify-work', 'bug-work', 'aggregate'):
        # Takes an output dir (every lane) or one lane dir.
        dirs = _lane_dirs(args.dir)
        for d in dirs:
            if len(dirs) > 1:
                print('== %s' % _lane(d))
            args.dir = str(d)
            cmds[args.cmd](args)
        return
    cmds[args.cmd](args)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3

# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Bookkeeping for the test-quality skill.

Agents do the judging; this script enumerates and shards the tests, tracks
progress, merges results into issue drafts, and posts them rate limited.

    tq.py init --type unittest [--path components/brave_wallet ...]
    tq.py status DIR
    tq.py verify-work DIR
    tq.py bug-work DIR
    tq.py aggregate DIR
    tq.py labels --repo brave/brave-browser
    tq.py post DIR --account netzenbot (--one PATH | --batch | --bugs)
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


def _fill(template, meta):
    text = (_SKILL_DIR / template).read_text(encoding='utf-8')
    for key in ('DIR', 'TYPE', 'TYPE_NAME', 'REPO_ROOT'):
        text = text.replace('{%s}' % key, str(meta[key.lower()]))
    return text


def cmd_init(args):
    type_name, file_re, test_re = TYPES[args.type]
    out = Path(
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
    sha = _run(['git', 'rev-parse', 'HEAD'], cwd=_REPO_ROOT).stdout.strip()
    meta = {
        'dir': str(out),
        'type': args.type,
        'type_name': type_name,
        'repo_root': str(_REPO_ROOT),
        'sha': sha,
        'paths': args.path or [],
    }
    _write_json(out / 'meta.json', meta)
    with open(out / 'manifest.tsv', 'w', encoding='utf-8') as f:
        f.write('path\tlines\ttests\n')
        for r in rows:
            f.write('%s\t%d\t%d\n' % r)
    shards = _shard(rows, args.max_lines)
    for i, shard in enumerate(shards, 1):
        with open(
            out / ('audit/shards/shard-%02d.txt' % i), 'w', encoding='utf-8'
        ) as f:
            for p, nlines, ntests in shard:
                f.write('%s\t%d lines\t%d tests\n' % (p, nlines, ntests))
    for src, dst in (
        ('audit_brief.md', 'AUDIT_BRIEF.md'),
        ('verify_brief.md', 'VERIFY_BRIEF.md'),
        ('bug_brief.md', 'BUG_BRIEF.md'),
    ):
        (out / dst).write_text(_fill(src, meta), encoding='utf-8')
    print('work dir: %s' % out)
    print(
        '%d files, %d lines, %d tests, %d audit shards'
        % (
            len(rows),
            sum(r[1] for r in rows),
            sum(r[2] for r in rows),
            len(shards),
        )
    )


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
    weak = sum(r.get('weak', 0) for r in files.values())
    out.append(
        'audit: %d/%d shards, %d files, %d weak tests'
        % (done, len(shards), len(files), weak)
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
    for p, r in sorted(_audit_rows(d).items()):
        findings = []
        for i, f in enumerate(r.get('findings', [])):
            findings.append(
                {
                    'id': '%s#%d' % (p, i),
                    'tests': f.get('tests', []),
                    'line': f.get('line'),
                    'category': f.get('category'),
                    'confidence': f.get('confidence'),
                    'claim': f.get('summary', ''),
                    'mutation': f.get('regression', ''),
                    'production_ref': f.get('production_ref', ''),
                    'suggested_fix': f.get('suggested_fix', ''),
                }
            )
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


def _test_issues(meta, verdicts, work):
    blob = 'https://github.com/brave/brave-core/blob/%s/' % meta['sha']
    files = collections.defaultdict(list)
    for i, v in verdicts.items():
        if v.get('verdict') in ('CONFIRMED', 'NARROWED') and i in work:
            files[work[i][0]].append(i)
    names = collections.Counter(os.path.basename(p) for p in files)
    issues = []
    for p, ids in sorted(files.items()):
        ids.sort(
            key=lambda i: (
                _SEV.get(verdicts[i].get('severity'), 3),
                work[i][1].get('line') or 0,
            )
        )
        top = verdicts[ids[0]].get('severity') or 'low'
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
                    v.get('severity') or 'low',
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
        feature = _feature_label(p)
        if feature:
            labels.append(feature)
        if p.startswith('ios/'):
            labels.append('OS/iOS')
        name = os.path.basename(p)
        if names[name] > 1:
            name = '/'.join(p.split('/')[-2:])
        issues.append(
            {
                'key': p,
                'title': 'Weak %s in %s' % (meta['type_name'], name),
                'body': '\n'.join(body) + '\n',
                'labels': labels,
                'severity': top,
                'findings': len(ids),
            }
        )
    issues.sort(key=lambda x: (_SEV.get(x['severity'], 3), x['key']))
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
    work = {}
    for w in sorted((d / 'verify/work').glob('shard-*.json')):
        for item in _read_json(w):
            for f in item['findings']:
                work[f['id']] = (item['path'], f)
    verdicts = _verify_rows(d)
    missing = sorted(set(work) - set(verdicts))
    audit = _audit_rows(d)
    print(
        'audit: %d files, %d tests reviewed, %d weak, %d findings'
        % (
            len(audit),
            sum(r.get('tests_reviewed', 0) for r in audit.values()),
            sum(r.get('weak', 0) for r in audit.values()),
            len(work),
        )
    )
    v = collections.Counter(
        verdicts[i].get('verdict') for i in work if i in verdicts
    )
    print('verify: %s | missing %d' % (dict(v), len(missing)))
    for m in missing[:10]:
        print('  missing:', m)

    issues = _test_issues(meta, verdicts, work)
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
            '1000',
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
        label = 'tests/test-quality'
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
    s.add_argument('--out', help='work dir (default under /tmp)')
    s.add_argument('--max-lines', type=int, default=9500)
    for name in ('status', 'bug-work', 'aggregate'):
        sub.add_parser(name).add_argument('dir')
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
    {
        'init': cmd_init,
        'status': cmd_status,
        'verify-work': cmd_verify_work,
        'bug-work': cmd_bug_work,
        'aggregate': cmd_aggregate,
        'labels': cmd_labels,
        'post': cmd_post,
    }[args.cmd](args)


if __name__ == '__main__':
    main()

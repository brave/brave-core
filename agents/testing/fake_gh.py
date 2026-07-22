#!/usr/bin/env python3

# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A stub `gh` for skill evals — serves canned GitHub data, records mutations.

Brave's `gh`/git-orchestration skills hit real GitHub. Evals must NOT touch
real state, so the provider puts this stub first on PATH (as an executable
named `gh`). It reads a canned-data JSON file from $FAKE_GH_DATA and answers the
read calls that `review-prs` makes, while RECORDING (never performing) any
mutating call to $FAKE_GH_MUTATIONS so a test can assert nothing was posted.

Canned-data schema ($FAKE_GH_DATA):
    {
      "repo": "brave/brave-core",
      "prs": {
        "1": {
          "title": "...", "author": "someuser", "state": "open",
          "merged": false, "mergeable": "MERGEABLE", "headRefOid": "abc123",
          "diff_file": "/abs/path/to/cs-003-violation.patch",
          "reviews": [], "review_comments": [], "issue_comments": []
        }
      }
    }

Only the calls review-prs actually makes are handled; everything else returns
empty/success so the skill's pipeline never crashes on an unstubbed call.
"""

import json
import os
import re
import sys


def _load_data():
    path = os.environ.get('FAKE_GH_DATA')
    if not path or not os.path.exists(path):
        return {}
    with open(path, encoding='utf-8') as f:
        return json.load(f)


def _record_mutation(argv):
    path = os.environ.get('FAKE_GH_MUTATIONS')
    if not path:
        return
    with open(path, 'a', encoding='utf-8') as f:
        f.write(json.dumps(argv) + '\n')


def _find_pr_number(args):
    for a in reversed(args):
        if a.isdigit():
            return a
    return None


def _pr_object(pr, number, repo):
    """Minimal GitHub REST pulls/{n} object with the fields review-prs reads."""
    return {
        'number': int(number),
        'title': pr.get('title', ''),
        'state': pr.get('state', 'open'),
        'merged': pr.get('merged', False),
        'mergeable': pr.get('mergeable', 'MERGEABLE'),
        'user': {
            'login': pr.get('author', 'external-user')
        },
        'head': {
            'sha': pr.get('headRefOid', 'deadbeefdeadbeefdeadbeef')
        },
        'html_url': f'https://github.com/{repo}/pull/{number}',
    }


def _pr_view_object(pr, number, repo):
    """`gh pr view --json` object with the fields fetch-prs.py requests."""
    return {
        'number': int(number) if number else 0,
        'title': pr.get('title', ''),
        'state': (pr.get('state') or 'open').upper(),
        'author': {
            'login': pr.get('author', 'external-user')
        },
        'isDraft': pr.get('isDraft', False),
        'headRefOid': pr.get('headRefOid', 'deadbeefdeadbeefdeadbeef'),
        'baseRefName': pr.get('baseRefName', 'master'),
        'reviewDecision': pr.get('reviewDecision', ''),
        'latestReviews': pr.get('latestReviews', []),
        'reviewRequests': pr.get('reviewRequests', []),
        'updatedAt': pr.get('updatedAt', '2026-01-01T00:00:00Z'),
        'mergeable': pr.get('mergeable', 'MERGEABLE'),
        'mergedAt': pr.get('mergedAt'),
        'url': f'https://github.com/{repo}/pull/{number}',
    }


def _is_mutation(args):
    if not args:
        return False
    # gh api with a write method or fields is a mutation; `pr review/comment/
    # merge/close`, `issue create`, etc. are mutations.
    if args[0] == 'api':
        if any(a in ('-X', '--method') for a in args):
            i = args.index('-X') if '-X' in args else args.index('--method')
            if i + 1 < len(args) and args[i + 1].upper() != 'GET':
                return True
        if any(a in ('-f', '-F', '--field', '--raw-field', '--input')
               for a in args):
            return True
        return False
    mutating_pr = {'review', 'comment', 'merge', 'close', 'edit', 'ready'}
    if args[0] == 'pr' and len(args) > 1 and args[1] in mutating_pr:
        return True
    if args[0] == 'issue' and len(args) > 1 and args[1] in {
            'create', 'comment', 'edit', 'close'
    }:
        return True
    return False


def main(argv):
    data = _load_data()
    repo = data.get('repo', 'brave/brave-core')
    prs = data.get('prs', {})

    if _is_mutation(argv):
        _record_mutation(argv)
        # Pretend it worked so the skill keeps going, but nothing is posted.
        print(json.dumps({'stubbed': True}))
        return 0

    if not argv:
        return 0

    # gh pr diff [--repo R] <N>
    if argv[0] == 'pr' and len(argv) > 1 and argv[1] == 'diff':
        number = _find_pr_number(argv)
        pr = prs.get(str(number), {})
        diff_file = pr.get('diff_file')
        if diff_file and os.path.exists(diff_file):
            with open(diff_file, encoding='utf-8') as f:
                sys.stdout.write(f.read())
            return 0
        sys.stderr.write(f'fake_gh: no diff for PR {number}\n')
        return 1

    # gh api <endpoint> [--paginate] ...
    if argv[0] == 'api':
        endpoint = next((a for a in argv[1:] if not a.startswith('-')), '')
        # gh api user --jq .login  (bot identity resolution)
        if endpoint == 'user':
            login = data.get('bot_username', 'brave-builds')
            if '--jq' in argv:
                print(login)
            else:
                print(json.dumps({'login': login}))
            return 0
        m = re.match(r'repos/[^/]+/[^/]+/pulls/(\d+)/reviews', endpoint)
        if m:
            print(json.dumps(prs.get(m.group(1), {}).get('reviews', [])))
            return 0
        m = re.match(r'repos/[^/]+/[^/]+/pulls/(\d+)/comments', endpoint)
        if m:
            print(
                json.dumps(prs.get(m.group(1), {}).get('review_comments', [])))
            return 0
        m = re.match(r'repos/[^/]+/[^/]+/issues/(\d+)/comments', endpoint)
        if m:
            print(json.dumps(
                prs.get(m.group(1), {}).get('issue_comments', [])))
            return 0
        m = re.match(r'repos/[^/]+/[^/]+/pulls/(\d+)$', endpoint)
        if m:
            print(
                json.dumps(
                    _pr_object(prs.get(m.group(1), {}), m.group(1), repo)))
            return 0
        # Unknown read endpoint: empty JSON so callers that json.loads() are ok.
        print('[]')
        return 0

    # gh pr view <N> [--repo R] [--json f1,f2,...]: build from canned data,
    # returning only the requested --json fields (as real `gh` does).
    if argv[0] == 'pr' and len(argv) > 1 and argv[1] == 'view':
        number = _find_pr_number(argv)
        pr = prs.get(str(number), {})
        full = _pr_view_object(pr, number, repo)
        if '--json' in argv:
            i = argv.index('--json')
            fields = argv[i + 1].split(',') if i + 1 < len(argv) else []
            full = {k: full.get(k) for k in fields}
        print(json.dumps(full))
        return 0

    # gh pr list: emit an empty array.
    if argv[0] == 'pr' and len(argv) > 1 and argv[1] == 'list':
        print('[]')
        return 0

    # Anything else: succeed quietly.
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

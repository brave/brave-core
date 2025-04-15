#!/usr/bin/env python3
# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
This script posts comments to a GitHub PRs based on the results of a presubmit
check.

Example:
    ./post_presubmit.py --input results.json --pr 1234


You can produce the JSON output for a presubmit run by using:
    npm run presubmit -- --base origin/master --json .presubmit_results.json
"""

import argparse
from pathlib import Path
import subprocess
import json
import sys
import hashlib
from typing import List, Dict, Any


def post_comments(presubmit_entries: Dict[str, List[Dict[str, Any]]],
                  pr_number: str) -> None:
    """
    Posts comments to a GitHub pull request based on presubmit results.

    Args:
        presubmit_entries:
            A dictionary containing presubmit results categorized by severity
            (e.g., 'errors', 'warnings', 'notifications').
        pr_number:
            The pull request number where the comments will be posted.
    """
    # Get a list of existing comments for this PR
    existing_comments: List[str] = json.loads(
        subprocess.check_output([
            'gh', 'api', f'repos/brave/brave-core/issues/{pr_number}/comments',
            '--paginate', '--jq', '[.[] | {id: .id, body: .body}]'
        ],
                                text=True))

    # Filtering out all comments that do not contain a presubmit hash as those
    # are not relevant to our comment management.
    existing_comments = [
        comment for comment in existing_comments
        if '<!-- presubmit-hash=' in comment["body"]
    ]

    active_report_hashes = set()
    for category, notifications in presubmit_entries.items():
        if category == 'notifications':
            # There's no need to report info messages to PRs, as they are not
            # actionable, and become noise.
            continue

        for report in notifications:
            # Hashing of the report is done so we can add it to the comment
            # itself as a html comment, and then check if it already exists,
            # so we don't post the same comment multiple times.
            report_hash = hashlib.sha256(
                json.dumps(report,
                           sort_keys=True).encode('utf-8')).hexdigest()
            comment_hash = f'<!-- presubmit-hash={report_hash} -->'
            active_report_hashes.add(report_hash)

            if any(comment_hash in comment["body"]
                   for comment in existing_comments):
                print(
                    f'Comment with hash {report_hash} already exists. Skipping.'
                )
                continue

            def get_severity_message():
                if category == 'errors':
                    # There's code never gets here as a presubmit error causes
                    # a CI failure, so the CI may end up never launching this
                    # script. Also, in a few tests, it has not been very clear
                    # whether or not PRESUBMIT generates JSON entries for
                    # errors.
                    return (
                        '> [!CAUTION]\n'
                        '> You have got a presubmit error. This will cause a '
                        'CI failure.')
                if category == 'warnings':
                    return (
                        '> [!WARNING]\n'
                        '> You have got a presubmit warning. Please address '
                        'it if possible.')

                raise ValueError(f'Unknown category: {category}')

            message = report.get('message', '')
            body_content = get_severity_message()
            if message:
                body_content += f'\n\n```\n{message}\n```'

            items = report.get('items', [])
            if items:
                body_content += '\n\nItems:\n```\n'
                for item in items:
                    body_content += f'{item}\n'
                body_content += '```'

            body_content += f'\n\n{comment_hash}'

            subprocess.run([
                'gh', 'api',
                f'repos/brave/brave-core/issues/{pr_number}/comments', '-X',
                'POST', '-F', 'body=@-'
            ],
                           input=body_content.encode('utf-8'),
                           check=True)

    # Now we delete the old comments that the hashes didn't come up.
    for comment in existing_comments:
        if '<!-- presubmit-hash=' not in comment["body"]:
            continue

        comment_hash = comment["body"].split('<!-- presubmit-hash=')[1].split(
            ' -->')[0]
        if comment_hash not in active_report_hashes:
            print(
                f'Deleting comment with id {comment["id"]}, hash {comment_hash}'
            )
            subprocess.check_call([
                'gh', 'api',
                f'repos/brave/brave-core/issues/comments/{comment["id"]}',
                '-X', 'DELETE'
            ])


def validate_pr_number(value: str) -> str:
    """  Validates that the provided PR number is a numeric value."""
    if not value.isdigit():
        raise argparse.ArgumentTypeError(
            'The --pr argument must be a numeric value.')
    return value


def main() -> int:
    parser = argparse.ArgumentParser(description='Post-presubmit script.')
    parser.add_argument(
        '--input',
        type=str,
        default='.presubmit_results.json',
        help='A JSON file with the output of the presubmit run (default: '
        '.presubmit_results.json)')
    parser.add_argument('--pr',
                        type=validate_pr_number,
                        required=True,
                        help='The PR number.')
    args = parser.parse_args()

    presubmit_path = Path(args.input)
    presubmit_entries: Dict[str, Any] = json.loads(presubmit_path.read_text())
    if not presubmit_entries:
        print('No presubmit entries found.')
        return 0

    post_comments(presubmit_entries, args.pr)

    return 0


if __name__ == '__main__':
    sys.exit(main())

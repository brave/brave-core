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

Notice: Keep this script as *standalone*, with no deps to `tools/cr` code, so
we can have no room for deps if downloaded and run on its own.
"""

from __future__ import annotations

import argparse
import logging
from pathlib import Path
import subprocess
import json
import sys
import hashlib
from typing import Any


def _check_call(*command,
                capture_stdout: bool = False,
                stdin: bytes | None = None) -> str | None:
    """Run *command* as a subprocess, logging the invocation.

    Logs the full command string at INFO level before executing it.  Stderr
    is inherited from the parent process so subprocess output streams
    directly to the terminal.

    Args:
        *command: The program and its arguments (passed as positional args,
            not as a list).
        capture_stdout: When `True`, capture the child's stdout and return it
            as a UTF-8 decoded string.  When `False` (default), stdout is
            inherited from the parent and the function returns `None`.
        stdin: Optional bytes to send to the child's stdin.

    Returns:
        The decoded stdout if `capture_stdout=True`, otherwise `None`.

    Raises:
        subprocess.CalledProcessError: If the process exits with a non-zero
            return code.
    """
    logging.info(' >>>> %s', ' '.join(str(a) for a in command))
    result = subprocess.run(command,
                            check=True,
                            input=stdin,
                            stdout=subprocess.PIPE if capture_stdout else None)
    if capture_stdout:
        return result.stdout.decode('utf-8', errors='replace')
    return None


def post_comments(presubmit_entries: dict[str, list[dict[str, Any]]],
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
    comments_response = _check_call(
        'gh',
        'api',
        f'repos/brave/brave-core/issues/{pr_number}/comments',
        '--paginate',
        '--jq',
        '[.[] | {id: .id, body: .body}]',
        capture_stdout=True)
    logging.debug('Existing comments response: %s', comments_response)
    existing_comments: list[dict[str, Any]] = []
    for line in comments_response.splitlines():
        if not line.strip():
            continue
        existing_comments.extend(json.loads(line))

    # Filtering out all comments that do not contain a presubmit hash as those
    # are not relevant to our comment management.
    existing_comments = [
        comment for comment in existing_comments
        if '<!-- presubmit-hash=' in comment["body"]
    ]

    active_report_hashes = set()
    for category, reports in presubmit_entries.items():
        if category == 'notifications':
            # There's no need to report info messages to PRs, as they are not
            # actionable, and become noise.
            continue

        for report in reports:
            # Check that the report has a message key
            if not isinstance(report, dict) or not 'message' in report:
                continue

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
                logging.info('Comment with hash %s already exists. Skipping.',
                             report_hash)
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

            _check_call('gh',
                        'api',
                        f'repos/brave/brave-core/issues/{pr_number}/comments',
                        '-X',
                        'POST',
                        '-F',
                        'body=@-',
                        stdin=body_content.encode('utf-8'))

    # Now we delete the old comments that the hashes didn't come up.
    for comment in existing_comments:
        if '<!-- presubmit-hash=' not in comment["body"]:
            continue

        comment_hash = comment["body"].split('<!-- presubmit-hash=')[1].split(
            ' -->')[0]
        if comment_hash not in active_report_hashes:
            logging.info('Deleting comment with id %s, hash %s', comment["id"],
                         comment_hash)
            _check_call(
                'gh', 'api',
                f'repos/brave/brave-core/issues/comments/{comment["id"]}',
                '-X', 'DELETE')


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
    parser.add_argument('--verbose',
                        action='store_true',
                        help='Enable verbose logging output.')
    args = parser.parse_args()
    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    presubmit_path = Path(args.input)
    presubmit_entries: dict[str, Any] = json.loads(
        presubmit_path.read_bytes().decode('utf-8'))
    if not presubmit_entries:
        logging.info('No presubmit entries found.')
        return 0

    post_comments(presubmit_entries, args.pr)

    return 0


if __name__ == '__main__':
    sys.exit(main())

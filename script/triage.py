#!/usr/bin/env python
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import print_function
from builtins import str
from builtins import object
from datetime import datetime, timezone
import argparse
import os
import re
import sys

#from lib.config import get_env_var, BRAVE_CORE_ROOT
#from lib.util import execute, scoped_cwd
from lib.helpers import channels, BRAVE_REPO, BRAVE_CORE_REPO
from lib.github import (GitHub, get_authenticated_user_login, parse_user_logins,
                        parse_labels, get_file_contents, get_milestones,
                        add_reviewers_to_pull_request, create_pull_request,
                        fetch_origin_check_staged, get_local_branch_name,
                        get_title_from_first_commit, push_branches_to_remote,
                        set_issue_details)


class TriageConfig():
    is_verbose = False
    is_dryrun = False
    github_token = None
    # team args
    parsed_team = []
    # stale args
    leave_comment = False
    close_issue = False
    ignore_comments = False
    ignore_priority = False
    limit = -1
    stale_months = 36
    ghost_only = False
    issues_expected = 30
    max_comments = 9
    max_reactions = 4

    def initialize(self, args):
        try:
            self.is_verbose = args.verbose
            self.is_dryrun = args.dry_run
            self.github_token = os.environ.get('GITHUB_TOKEN')

            # team report
            self.parsed_team = parse_user_logins(self.github_token,
                                                 args.team,
                                                 verbose=self.is_verbose)
            # stale script
            self.leave_comment = args.leave_comment
            self.close_issue = args.close_issue
            self.ignore_comments = args.ignore_comments
            self.ignore_priority = args.ignore_priority
            if args.limit:
                self.limit = int(args.limit)
            if args.months:
                self.stale_months = int(args.months)
            self.ghost_only = args.ghost_only
            if args.issues_expected:
                self.issues_expected = int(args.issues_expected)
            if args.max_comments:
                self.max_comments = int(args.max_comments)
            if args.max_reactions:
                self.max_reactions = int(args.max_reactions)

            if self.is_verbose:
                print('[INFO] config: ' + str(vars(self)))
            return 0
        except Exception as e:
            print(
                '[ERROR] error returned from GitHub API while initializing ' +
                'config: ' + str(e))
            return 1


config = TriageConfig()

def parse_args():
    parser = argparse.ArgumentParser(
        description='automation for triaging `brave-browser` issues')

    # team report
    parser.add_argument(
        '--team',
        help='run the "team" script. Comma seperated list of GitHub logins',
        default=None)

    # stale script
    parser.add_argument(
        '--stale',
        help='run the "stale" script',
        action='store_true',
        default=None)
    parser.add_argument(
        '--leave-comment',
        help='leave a comment when running "stale" script',
        action='store_true',
        default=False)
    parser.add_argument(
        '--close-issue',
        help='close issue when running "stale" script',
        action='store_true',
        default=False)
    parser.add_argument(
        '--ignore-comments',
        help='consider issues which have more than 0 comments',
        action='store_true',
        default=False)
    parser.add_argument(
        '--ignore-priority',
        help='consider issues which already have priority label',
        action='store_true',
        default=False)
    parser.add_argument(
        '--limit',
        help='maximum number of issues to modify (if applicable)',
        default=None)
    parser.add_argument(
        '--months',
        help='number of months without activity before an issue is considered stale (default: 36)',
        default=None)
    parser.add_argument(
        '--ghost-only',
        help='only consider issues created by deleted accounts when running "stale" script',
        action='store_true',
        default=False)
    parser.add_argument(
        '--issues-expected',
        help='minimum number of stale issues to collect before processing (default: 30)',
        default=None)
    parser.add_argument(
        '--max-comments',
        help='maximum number of comments an issue can have to be considered stale (default: 9)',
        default=None)
    parser.add_argument(
        '--max-reactions',
        help='maximum number of reactions an issue can have to be considered stale (default: 4)',
        default=None)

    # parser.add_argument(
    #     '--crashes',
    #     help='run the "crashes" script',
    #     action='store_true',
    #     default=None)

    # general
    parser.add_argument('-v',
                        '--verbose',
                        action='store_true',
                        help='prints the output of the GitHub API calls')
    parser.add_argument(
        '-n',
        '--dry-run',
        action='store_true',
        help=
        'don\'t perform any changes; just show which actions would be taken')

    return parser.parse_args()


def get_issues_for_team_member(login):
    try:
        # most efficient API for searching for count
        get_data = {
          'q': 'repo:' + BRAVE_REPO + ' is:issue is:open assignee:' + login
        }
        response = GitHub(config.github_token).search().issues().get(params=get_data)
        total_count = int(response['total_count'])
    except Exception as e:
        # this can happen if account is set as private
        # if person has > 100, we'd need to page to find count
        get_data = {
          'assignee': login,
          'state': 'open',
          'per_page': 100
        }
        repo = GitHub(config.github_token).repos(BRAVE_REPO)
        response = repo.issues().get(params=get_data)
        total_count = len(response)
        # TODO: put count of P1 and P2 issues!
    print('- ' + str(total_count) + ' issues assigned. https://github.com/brave/brave-browser/issues?q=is%3Aissue%20state%3Aopen%20assignee%3A' + login)


def get_prs_for_team_member(login):
    try:
        # Most efficient API for searching for count
        get_data = {
          'q': 'repo:' + BRAVE_CORE_REPO + ' is:pr is:open assignee:' + login
        }
        response = GitHub(config.github_token).search().issues().get(params=get_data)
        total_count = int(response['total_count'])
    except Exception as e:
        # This condition can be hit if the GitHub account is set as private.
        # NOTE: If person has > 100 open issues, we'd need to implement paging.
        get_data = {
          'assignee': login,
          'state': 'open',
          'per_page': 100
        }
        repo = GitHub(config.github_token).repos(BRAVE_CORE_REPO)
        response = repo.issues().get(params=get_data)
        total_count = len(response)
    print('- ' + str(total_count) + ' open PRs. https://github.com/brave/brave-core/pulls/assigned/' + login)


def get_stale_issues(page=1):
    get_data = {
      'state': 'open',
      'sort': 'updated',
      'direction': 'asc',
      'per_page': 100,
      'page': page
    }
    repo = GitHub(config.github_token).repos(BRAVE_REPO)

    get_headers = dict()
    response = repo.issues().get(params=get_data, headers=get_headers)

    link_header = get_headers['ResponseHeaders']['Link']
    has_pages_remaining = link_header.find('rel="next"') != -1

    stale_issues = []
    now = datetime.now(timezone.utc)
    cutoff_month = now.month - (config.stale_months % 12)
    cutoff_year = now.year - (config.stale_months // 12)
    if cutoff_month <= 0:
        cutoff_month += 12
        cutoff_year -= 1
    cutoff_date = now.replace(year=cutoff_year, month=cutoff_month)

    for issue in response:
        # Search only issues opened by ghost (deleted account).
        if config.ghost_only:
            if issue['user'] is not None and issue['user']['login'] != 'ghost':
                print('[INFO] Issue ' + str(issue['number']) + ' has a valid author (not ghost account); skipping.' + issue['html_url'])
                continue
        # By default, skip any issues with comments.
        # Override by providing --ignore-comments
        if not config.ignore_comments and issue['comments'] > 0:
            print('[INFO] Issue ' + str(issue['number']) + ' has comments (' + str(issue['comments']) + '); skipping.' + issue['html_url'])
            continue
        # By default, skip any issues with priority set.
        # Override by providing --ignore-priority
        if not config.ignore_priority:
            can_add_issue = True
            for label in issue['labels']:
                if label['name'].startswith('priority'):
                    can_add_issue = False
                    break
            if not can_add_issue:
                print('[INFO] Issue ' + str(issue['number']) + ' has a priority label on it (' + label['name'] + '); skipping. ' + issue['html_url'])
                continue
        # safeguard: Don't triage issues with too many comments.
        if issue['comments'] > config.max_comments:
            print('[INFO] Skipping issue ' + str(issue['number']) + ' as it has more than ' + str(config.max_comments) + ' comments. ' + issue['html_url'])
            continue
        # safeguard: Don't triage issues with too many reactions.
        if issue['reactions']['total_count'] > config.max_reactions:
            print('[INFO] Skipping issue ' + str(issue['number']) + ' as it has more than ' + str(config.max_reactions) + ' reactions. ' + issue['html_url'])
            continue
        # Label safeguards:
        skip_issue = False
        for label in issue['labels']:
            # Don't close P1/P2 issues
            if label['name'] in ['priority/P1', 'priority/P2']:
                print('[INFO] Skipping issue as it\'s flagged as P1 or P2. ' + issue['html_url'])
                skip_issue = True
                break
            # don't close code health issues
            if label['name'] in ['dev-concern', 'ci-concern']:
                skip_issue = True
                print('[INFO] Skipping issue as it\'s a code health issue. ' + issue['html_url'])
                break
        if skip_issue:
            continue

        issue_updated_at = datetime.fromisoformat(issue['updated_at'].replace("Z", "+00:00"))
        if issue_updated_at < cutoff_date:
            stale_issues.append(issue['number'])
        if config.is_verbose:
            print('[INFO] ' + issue['title'] + ' (' + str(issue['comments']) + ' comments) ' + issue['html_url'])

    return stale_issues, has_pages_remaining


def process_stale_issue(issue_number):
    repo = GitHub(config.github_token).repos(BRAVE_REPO)
    response = repo.issues(issue_number).get()

    print('\nProcessing ' + str(response['html_url']))
    patch_data = {
        'labels': []
    }

    # Add the stale label, remove specific labels if closing
    labels_to_remove = {'priority/P3', 'priority/P4', 'priority/P5', 'help wanted', 'good first issue'} if config.close_issue else set()
    for label in response['labels']:
        if label['name'] not in labels_to_remove:
            patch_data['labels'].append(label['name'])
    patch_data['labels'].append('closed/stale')

    # (optional) Close the issue
    if config.close_issue:
        patch_data['state'] = 'closed'

    # (optional) Add a comment
    post_data = {}
    if config.leave_comment:
        post_data['body'] = 'This issue hasn\'t had an update in a while - it\'s met the criteria for being stale. '
        post_data['body'] += 'The issue is going to be closed for now. If this is still a problem, please leave a comment'
        if config.close_issue:
            post_data['body'] += ' or re-open the issue'
        post_data['body'] += '. Thanks!'

        if config.is_dryrun:
            print('- Would leave comment')
            if config.is_verbose:
                print('[INFO] would call `repo.issues(' + str(issue_number) +
                      ').comments.post(' + str(post_data) + ')`')
        else:
            print('- Leaving comment')
            if config.is_verbose:
                print('[INFO] calling `repo.issues(' + str(issue_number) +
                      ').comments.post(' + str(post_data) + ')`')
            response = repo.issues(issue_number).comments.post(data=post_data)

    if config.is_dryrun:
        print('- Would add stale label')
        if config.close_issue:
            removed = labels_to_remove & {label['name'] for label in response['labels']}
            if removed:
                print('- Would remove labels: ' + ', '.join(sorted(removed)))
            print('- Would close issue')
        if config.is_verbose:
            print('[INFO] would call `repo.issues(' + str(issue_number) +
                  ').patch(' + str(patch_data) + ')`')
        return

    # Update the issue
    try:
        if config.is_verbose:
            print('[INFO] calling `repo.issues(' + str(issue_number) +
                  ').patch(' + str(patch_data) + ')`')
        response = repo.issues(issue_number).patch(data=patch_data)
        print('- Added stale label')
        if config.close_issue:
            print('- Closed issue')
    except Exception as e:
        print('[ERROR] ' + str(e))


def main():
    args = parse_args()
    if args.verbose:
        print('[INFO] args: ' + str(args))

    global config
    result = config.initialize(args)
    if result != 0:
        return result

    if len(config.parsed_team) > 0:
        print('Team report')
        print('-----------')
        for login in config.parsed_team:
            print('\n' + login + ':')
            get_issues_for_team_member(login)
            get_prs_for_team_member(login)
        return 0

    if args.stale:
        print('Stale issue cleanup')
        print('-------------------')

        # Paging implemented to ensure we get a minimum number of issues
        # AFTER filtering is applied.
        issues_found = 0
        page = 1
        issues, has_pages_remaining = get_stale_issues()
        issues_found += len(issues)
        print('(page 1) Got ' + str(len(issues)) + ' issues (' + str(issues_found) + ' total).\n')

        # Code will continue making requests until it finds the minimum number
        # of issues or there are no more result pages.
        while issues_found < config.issues_expected and has_pages_remaining:
            page = page + 1
            more_issues, has_pages_remaining = get_stale_issues(page)
            print('(page ' + str(page) + ') Got ' + str(len(more_issues)) + ' more issues (' + str(issues_found) + ' total).\n')
            issues_found += len(more_issues)
            issues += more_issues

        print('\nFound ' + str(len(issues)) + ' actionable issues.')
        issue_count = 0
        for issue_number in issues:
            process_stale_issue(issue_number)
            issue_count += 1
            if config.limit > 0 and issue_count >= config.limit:
                print('\nLimit of ' + str(config.limit) + ' hit. Exiting')
                return 0
        return 0

    return 0


if __name__ == '__main__':
    sys.exit(main())

#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import errno
import hashlib
import os
import requests
import re
import shutil
import subprocess
import sys
import tempfile
import base64
import json

from io import StringIO
from lib.config import get_env_var, SOURCE_ROOT, BRAVE_CORE_ROOT, get_raw_version
from lib.util import execute, scoped_cwd
from lib.helpers import *
from lib.github import GitHub


channel_names = channels()
is_verbose = False
is_dryrun = False
channel_branches = {}
branches_to_push = []


# TODOs for future versions
###########################
# - also submit PRs for brave-browser (not only brave-core)
# - if any failures happen, clean up if needed
# - validate GitHub logins
# - validate channel name (passed with --uplift-to)


def main():
    args = parse_args()
    is_verbose = args.verbose
    is_dryrun = args.dry_run
    print(args)

    with scoped_cwd(BRAVE_CORE_ROOT):
        execute(['git', 'fetch', 'origin'])
        # TODO: check if there are unstaged changes. Error out.

    # Repo is defined in lib/helpers.py for now
    github_token = get_env_var('GITHUB_TOKEN')
    brave_repo = GitHub(github_token).repos(BRAVE_REPO)
    brave_core_repo = GitHub(github_token).repos(BRAVE_CORE_REPO)

    # get local version + latest version on remote (master)
    # for more info see: http://developer.github.com/v3/repos/contents
    local_version = get_raw_version()
    file = brave_repo.contents("package.json").get()
    decoded_file = base64.b64decode(file['content'])
    json_file = json.loads(decoded_file)
    remote_nightly_version = json_file['version']

    # if they don't match, rebase is needed
    if local_version != remote_nightly_version:
        print('[ERROR] Your branch is out of sync (local=' + local_version +
              ', remote=' + remote_nightly_version + '); please rebase against master')
        return 1

    # get the branch name and the first commit subject (used as the title of the pull request)
    with scoped_cwd(BRAVE_CORE_ROOT):
        local_branch = execute(['git', 'rev-parse', '--abbrev-ref', 'HEAD']).strip()
        title_list = execute(['git', 'log', 'origin/master..HEAD', '--pretty=format:%s', '--reverse'])
        title_list = title_list.split('\n')
        if len(title_list) == 0:
            # TODO: throw exception
            print('whoops- no commits?!')
        title = title_list[0]

    # Create a branch for each channel
    print('\nCreating branches...')
    base = get_channel_bases(remote_nightly_version)
    try:
        for channel in channel_names:
            branch = create_branch(channel, base[channel], local_branch)
            channel_branches[channel] = branch
            if channel == args.uplift_to:
                break
    except Exception as e:
        print('[ERROR] cherry-pick failed for branch "' + branch + '". Please resolve manually.')
        return 1

    print('\nPushing local branches to remote...')
    with scoped_cwd(BRAVE_CORE_ROOT):
        for branch_to_push in branches_to_push:
            # TODO: if they already exist, force push?? or error??
            execute(['git', 'push', '-u', 'origin', branch_to_push])

    print('\nCreating the pull requests...')
    for channel in channel_names:
        submit_pr(
            brave_core_repo,
            channel, title,
            base[channel],
            channel_branches[channel],
            args)
        if channel == args.uplift_to:
            break

    print('\nDone!')

    return 0


# given something like "0.60.2", return branch version ("0.60.x")
def get_current_version_branch(version):
    version = str(version)
    if version[0] == 'v':
        version = version[1:]
    parts = version.split('.', 3)
    parts[2] = 'x'
    return '.'.join(parts)


# given something like "0.60.x", get previous version ("0.59.x")
def get_previous_version_branch(version):
    version = str(version)
    if version[0] == 'v':
        version = version[1:]
    parts = version.split('.', 3)
    parts[1] = str(int(parts[1]) - 1)
    parts[2] = 'x'
    return '.'.join(parts)


def get_channel_bases(raw_nightly_version):
    nightly_version = get_current_version_branch(raw_nightly_version)
    dev_version = get_previous_version_branch(nightly_version)
    beta_version = get_previous_version_branch(dev_version)
    release_version = get_previous_version_branch(beta_version)
    return {
        channel_names[0]: nightly_version,
        channel_names[1]: dev_version,
        channel_names[2]: beta_version,
        channel_names[3]: release_version
    }


def create_branch(channel, base, local_branch):
    if channel != channel_names[0]:
        channel_branch = base + '_' + local_branch

        with scoped_cwd(BRAVE_CORE_ROOT):
            # get SHA for all commits (in order)
            sha_list = execute(['git', 'log', 'origin/master..HEAD', '--pretty=format:%h', '--reverse'])
            sha_list = sha_list.split('\n')
            try:
                # check if branch exists already
                try:
                    branch_sha = execute(['git', 'rev-parse', '-q', '--verify', channel_branch])
                except Exception as e:
                    branch_sha = ''

                if len(branch_sha) > 0:
                    # branch exists; reset it
                    print('(' + channel + ') branch "' + channel_branch + '" exists; resetting to origin/' + base)
                    execute(['git', 'reset', '--hard', 'origin/' + base])
                else:
                    # create the branch
                    print('(' + channel + ') creating "' + channel_branch + '" from ' + channel)
                    execute(['git', 'checkout', base])
                    execute(['git', 'checkout', '-b', channel_branch])

                # TODO: handle errors thrown by cherry-pick
                for sha in sha_list:
                    output = execute(['git', 'cherry-pick', sha]).split('\n')
                    print('- picked ' + sha + ' (' + output[0] + ')')

            finally:
                # switch back to original branch
                execute(['git', 'checkout', local_branch])
                execute(['git', 'reset', '--hard', sha_list[-1]])

        branches_to_push.append(channel_branch)

        return channel_branch
    return local_branch


def submit_pr(repo, channel, title, base, branch, args):
    try:
        # get the actual milestone this should be in
        # for more info see: https://developer.github.com/v3/issues/milestones/
        response = repo.milestones.get()
        if is_verbose:
            print('repo.milestones.get() response:\n' + str(response))
        for milestone in response:
            if milestone['title'].startswith(base + ' - '):
                milestone_number = milestone['number']
                break

        if milestone_number is None:
            print('milestone for "' + base + '"" was not found!')
            return 0

        pr_details = {
            'title': title,
            'head': branch,
            'base': base,
            'body': 'test 123',
            'maintainer_can_modify': True
        }

        if channel == channel_names[0]:
            pr_details['base'] = 'master'
        else:
            pr_details['title'] += ' (uplift to ' + base + ')'
            pr_details['body'] = 'Uplift of X'

        # create the pull request
        # for more info see: http://developer.github.com/v3/pulls
        print('(' + channel + ') creating pull request')
        if is_dryrun:
            print('[INFO] would call `repo.pulls.post(' + str(pr_details) + ')`')
            response = {'number': 123}
        else:
            response = repo.pulls.post(data=pr_details)
            if is_verbose:
                print('repo.pulls.post(data) response:\n' + str(response))

        # add reviewers to pull request
        # for more info see: https://developer.github.com/v3/pulls/review_requests/
        number = int(response['number'])
        patched_data = {}
        parsed_reviewers = parse_logins(args.reviewers)
        if len(parsed_reviewers) > 0:
            patched_data['reviewers'] = parsed_reviewers

        if is_dryrun:
            print('[INFO] would call `repo.pulls(' + number + ').requested_reviewers.post(' + str(patched_data) + ')`')
        else:
            response = repo.pulls(number).requested_reviewers.post(data=patched_data)
            if is_verbose:
                print('repo.pulls(' + number + ').requested_reviewers.post(data) response:\n' + str(response))

        # add milestone and assignee to pull request
        # for more info see: https://developer.github.com/v3/issues/#edit-an-issue
        patched_data = {
            'milestone': milestone_number
        }
        parsed_assignees = parse_logins(args.owners)
        if len(parsed_assignees) > 0:
            patched_data['assignees'] = parsed_assignees
        if is_dryrun:
            print('[INFO] would call `repo.issues(' + number + ').patch(' + str(patched_data) + ')`')
        else:
            response = repo.issues(number).patch(data=patched_data)
            if is_verbose:
                print('repo.issues(' + number + ').patch(data) response:\n' + str(response))

        return 0

    except Exception as e:
        print('[ERROR] unhandled error occurred:', str(e))


def parse_logins(logins):
    if logins is None:
        return []
    logins = logins.replace(" ", "")
    parsed_logins = logins.split(',')

    # TODO: validate logins are correct
    # https://developer.github.com/v3/users/#get-a-single-user

    return parsed_logins


def parse_args():
    parser = argparse.ArgumentParser(description='create PRs for all branches given branch against master')
    parser.add_argument('--reviewers',
                        help='comma separated list of GitHub logins to mark as reviewers',
                        default=None)
    parser.add_argument('--owners',
                        help='comma seperated list of GitHub logins to mark as assignee',
                        default=None)
    parser.add_argument('--uplift-to',
                        help='starting at nightly (master), how far back to uplift the changes',
                        default='nightly')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='prints the output of the GitHub API calls')
    parser.add_argument('-n', '--dry-run', action='store_true',
                        help='don\'t actually create pull requests; just show a call would be made')

    return parser.parse_args()


if __name__ == '__main__':
    import sys
    sys.exit(main())

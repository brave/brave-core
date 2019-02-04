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
import json

from io import StringIO
from lib.config import get_env_var, SOURCE_ROOT, BRAVE_CORE_ROOT, get_raw_version
from lib.util import execute, scoped_cwd
from lib.helpers import *
from lib.github import (GitHub, get_authenticated_user_login, parse_user_logins,
                        get_file_contents, add_reviewers_to_pull_request,
                        get_milestones, create_pull_request, set_issue_details,
                        fetch_origin_check_staged, push_branches_to_remote)


# TODOs for this version
#####
# - validate channel name (passed with --uplift-to)
# - parse out issue (so it can be included in body). ex: git log --pretty=format:%b
# - add arg for overriding the title
# - open PRs in default browser (shell exec) BY DEFAULT. Offer a quiet that DOESN'T open
# - discover associated issues
#    - put them in the uplift / original PR body
#    - set milestone! (on the ISSUE)
# - --labels=QA/Yes (and then put those on the issues). And check existance of label


class PrConfig:
    channel_names = channels()
    channels_to_process = channels()
    is_verbose = False
    is_dryrun = False
    branches_to_push = []
    master_pr_number = -1
    github_token = None
    parsed_reviewers = []
    parsed_owners = []
    milestones = None

    def initialize(self, args):
        try:
            self.is_verbose = args.verbose
            self.is_dryrun = args.dry_run
            # TODO: read github token FIRST from CLI, then from .npmrc
            self.github_token = get_env_var('GITHUB_TOKEN')
            if len(self.github_token) == 0:
                # TODO: check .npmrc
                # if not there, error out!
                print('[ERROR] no valid GitHub token was found')
                return 1
            self.parsed_reviewers = parse_user_logins(self.github_token, args.reviewers, verbose=self.is_verbose)
            # if `--owners` is not provided, fall back to user owning token
            self.parsed_owners = parse_user_logins(self.github_token, args.owners, verbose=self.is_verbose)
            if len(self.parsed_owners) == 0:
                self.parsed_owners = [get_authenticated_user_login(self.github_token)]
            if self.is_verbose:
                print('[INFO] config: ' + str(vars(self)))
            return 0
        except Exception as e:
            print('[ERROR] error returned from GitHub API while initializing config: ' + str(e))
            return 1


config = PrConfig()


def is_nightly(channel):
    global config
    return config.channel_names[0] == channel


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


def get_remote_channel_branches(raw_nightly_version):
    global config
    nightly_version = get_current_version_branch(raw_nightly_version)
    dev_version = get_previous_version_branch(nightly_version)
    beta_version = get_previous_version_branch(dev_version)
    release_version = get_previous_version_branch(beta_version)
    return {
        config.channel_names[0]: nightly_version,
        config.channel_names[1]: dev_version,
        config.channel_names[2]: beta_version,
        config.channel_names[3]: release_version
    }


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
    parser.add_argument('--start-from',
                        help='instead of starting from nightly (default), start from beta/dev/release',
                        default='nightly')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='prints the output of the GitHub API calls')
    parser.add_argument('-n', '--dry-run', action='store_true',
                        help='don\'t actually create pull requests; just show a call would be made')

    return parser.parse_args()


def get_versions(args):
    global config
    local_version = get_raw_version()
    if not is_nightly(args.start_from):
        # TODO: fix me
        target_branch = '0.60.x'
    else:
        target_branch = 'master'
    decoded_file = get_file_contents(config.github_token, BRAVE_REPO, 'package.json', target_branch)
    json_file = json.loads(decoded_file)
    remote_version = json_file['version']
    return local_version, remote_version


def main():
    args = parse_args()
    if args.verbose:
        print('[INFO] args: ' + str(args))

    global config
    result = config.initialize(args)
    if result != 0:
        return result

    result = fetch_origin_check_staged(BRAVE_CORE_ROOT)
    if result != 0:
        return result

    # get local version + latest version on remote (master)
    # if they don't match, rebase is needed
    # TODO: allow for diff NOT against master; in that case, use the start-from branch!
    local_version, remote_version = get_versions(args)
    if local_version != remote_version:
        print('[ERROR] Your branch is out of sync (local=' + local_version +
              ', remote=' + remote_version + '); please rebase against master')
        return 1

    # get the branch name and the first commit subject (used as the title of the pull request)
    # TODO: allow for diff NOT against master; in that case, use the start-from branch!
    with scoped_cwd(BRAVE_CORE_ROOT):
        local_branch = execute(['git', 'rev-parse', '--abbrev-ref', 'HEAD']).strip()
        title_list = execute(['git', 'log', 'origin/master..HEAD', '--pretty=format:%s', '--reverse'])
        title_list = title_list.split('\n')
        if len(title_list) == 0:
            print('[ERROR] No commits found!')
            return 1
        title = title_list[0]

    # if starting point is NOT nightly, remove options which aren't desired
    if not is_nightly(args.start_from):
        try:
            start_index = config.channel_names.index(args.start_from)
            config.channels_to_process = config.channel_names[start_index:]
        except Exception as e:
            print('[ERROR] specified `start-from` value "' + args.start_from + '" not found in channel list')
            return 1

    # Create a branch for each channel
    print('\nCreating branches...')
    remote_branches = get_remote_channel_branches(local_version)
    local_branches = {}
    try:
        for channel in config.channels_to_process:
            branch = create_branch(channel, remote_branches[channel], local_branch)
            local_branches[channel] = branch
            if channel == args.uplift_to:
                break
    except Exception as e:
        print('[ERROR] cherry-pick failed for branch "' + branch + '". Please resolve manually:\n' + str(e))
        return 1

    print('\nPushing local branches to remote...')
    push_branches_to_remote(BRAVE_CORE_ROOT, config.branches_to_push, dryrun=config.is_dryrun)

    try:
        print('\nCreating the pull requests...')
        for channel in config.channels_to_process:
            submit_pr(
                channel,
                title,
                remote_branches[channel],
                local_branches[channel])
            if channel == args.uplift_to:
                break
        print('\nDone!')
    except Exception as e:
        print('\n[ERROR] Unhandled error while creating pull request; ' + str(e))
        return 1

    return 0


def create_branch(channel, remote_branch, local_branch):
    global config

    if not is_nightly(channel):
        channel_branch = remote_branch + '_' + local_branch

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
                    print('(' + channel + ') branch "' + channel_branch + '" exists; resetting to origin/' + remote_branch)
                    execute(['git', 'reset', '--hard', 'origin/' + remote_branch])
                else:
                    # create the branch
                    print('(' + channel + ') creating "' + channel_branch + '" from ' + channel)
                    execute(['git', 'checkout', remote_branch])
                    execute(['git', 'checkout', '-b', channel_branch])

                # TODO: handle errors thrown by cherry-pick
                for sha in sha_list:
                    output = execute(['git', 'cherry-pick', sha]).split('\n')
                    print('- picked ' + sha + ' (' + output[0] + ')')

            finally:
                # switch back to original branch
                execute(['git', 'checkout', local_branch])
                execute(['git', 'reset', '--hard', sha_list[-1]])

        config.branches_to_push.append(channel_branch)

        return channel_branch
    return local_branch


def get_milestone_for_branch(channel_branch):
    global config
    if not config.milestones:
        config.milestones = get_milestones(config.github_token, BRAVE_CORE_REPO)
    for milestone in config.milestones:
        if milestone['title'].startswith(channel_branch + ' - '):
            return milestone['number']
    return None


def submit_pr(channel, title, remote_branch, local_branch):
    global config

    try:
        milestone_number = get_milestone_for_branch(remote_branch)
        if milestone_number is None:
            print('milestone for "' + remote_branch + '"" was not found!')
            return 0

        print('(' + channel + ') creating pull request')
        pr_title = title
        pr_dst = remote_branch
        if is_nightly(channel):
            pr_dst = 'master'
            pr_body = 'TODO: fill me in\n(created using `npm run pr`)'
        else:
            pr_title += ' (uplift to ' + remote_branch + ')'
            pr_body = 'Uplift of #' + str(config.master_pr_number)
        number = create_pull_request(config.github_token, BRAVE_CORE_REPO, pr_title, pr_body,
                                     branch_src=local_branch, branch_dst=pr_dst,
                                     verbose=config.is_verbose, dryrun=config.is_dryrun)

        # store the PR number (master only) so that it can be referenced in uplifts
        if is_nightly(channel):
            config.master_pr_number = number

        # assign milestone / reviewer(s) / owner(s)
        add_reviewers_to_pull_request(config.github_token, BRAVE_CORE_REPO, number, config.parsed_reviewers,
                                      verbose=config.is_verbose, dryrun=config.is_dryrun)
        set_issue_details(config.github_token, BRAVE_CORE_REPO, number, milestone_number, config.parsed_owners,
                          verbose=config.is_verbose, dryrun=config.is_dryrun)
    except Exception as e:
        print('[ERROR] unhandled error occurred:', str(e))


if __name__ == '__main__':
    import sys
    sys.exit(main())

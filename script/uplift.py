#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

from __future__ import print_function
from builtins import str
from builtins import object
import argparse
import re
import sys
import json

from lib.config import get_env_var, BRAVE_CORE_ROOT
from lib.util import execute, scoped_cwd
from lib.helpers import *
from lib.github import (GitHub, get_authenticated_user_login, parse_user_logins,
                        parse_labels, get_file_contents, get_milestones,
                        add_reviewers_to_pull_request, create_pull_request,
                        fetch_origin_check_staged, get_local_branch_name,
                        get_title_from_first_commit, push_branches_to_remote,
                        set_issue_details)


class PrConfig(object):
    channel_names = channels()
    channels_to_process = channels()
    is_verbose = False
    is_dryrun = False
    branches_to_push = []
    master_pr_number = -1
    github_token = None
    team_reviewers = ['uplift-approvers']
    parsed_owners = []
    milestones = None
    title = None
    labels = []

    def initialize(self, args):
        try:
            self.is_verbose = args.verbose
            self.is_dryrun = args.dry_run
            self.title = args.title
            # validate channel names
            validate_channel(args.uplift_to)
            validate_channel(args.start_from)
            # read github token FIRST from CLI, then from .npmrc
            self.github_token = get_env_var('GITHUB_TOKEN')
            if len(self.github_token) == 0:
                try:
                    npm_cmd = 'npm'
                    if sys.platform.startswith('win'):
                        npm_cmd = 'npm.cmd'
                    result = execute(
                        [npm_cmd, 'config', 'get', 'BRAVE_GITHUB_TOKEN']).strip()
                    if result == 'undefined':
                        raise Exception(
                            '`BRAVE_GITHUB_TOKEN` value not found!')
                    self.github_token = result
                except Exception:
                    print('[ERROR] no valid GitHub token was found either in npmrc or ' +
                          'via environment variables (BRAVE_GITHUB_TOKEN)')
                    return 1
            # if `--owners` is not provided, fall back to user owning token
            self.parsed_owners = parse_user_logins(
                self.github_token, args.owners, verbose=self.is_verbose)
            if len(self.parsed_owners) == 0:
                self.parsed_owners = [
                    get_authenticated_user_login(self.github_token)]
            self.labels = parse_labels(self.github_token, BRAVE_CORE_REPO,
                                       args.labels, verbose=self.is_verbose)
            if self.is_verbose:
                print('[INFO] config: ' + str(vars(self)))
            return 0
        except Exception as e:
            print(
                '[ERROR] error returned from GitHub API while initializing config: ' + str(e))
            return 1


config = PrConfig()


def is_nightly(channel):
    global config
    return config.channel_names[0] == channel


# given something like "1.5.2", return branch version ("1.5.x")
def get_current_version_branch(version):
    version = str(version)
    if version[0] == 'v':
        version = version[1:]
    parts = version.split('.', 3)
    parts[2] = 'x'
    return '.'.join(parts)


# given something like "1.6.x", get previous version ("1.5.x")
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
    beta_version = get_previous_version_branch(nightly_version)
    release_version = get_previous_version_branch(beta_version)
    return {
        config.channel_names[0]: nightly_version,
        config.channel_names[1]: beta_version,
        config.channel_names[2]: release_version
    }


def validate_channel(channel):
    global config
    try:
        config.channel_names.index(channel)
    except Exception:
        raise Exception('Channel name "' + channel + '" is not valid!')


def parse_args():
    parser = argparse.ArgumentParser(
        description='create PRs for all branches given branch against master')
    parser.add_argument('-g', '--gpgsign',
                        help='GPG sign GitHub commit', action='store_true')
    parser.add_argument('--owners',
                        help='comma seperated list of GitHub logins to mark as assignee',
                        default=None)
    parser.add_argument('--uplift-to',
                        help='starting at nightly (master), how far back to uplift the changes',
                        default='beta')
    parser.add_argument('--uplift-using-pr',
                        help='link to already existing pull request (number) to use as a reference for uplifting',
                        required=True)
    parser.add_argument('--start-from',
                        help='instead of starting from nightly (default), start from beta/release',
                        default='beta')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='prints the output of the GitHub API calls')
    parser.add_argument('-n', '--dry-run', action='store_true',
                        help='don\'t actually create pull requests; just show a call would be made')
    parser.add_argument('--labels',
                        help='comma seperated list of labels to apply to each pull request',
                        default=None)
    parser.add_argument('--title',
                        help='title to use (instead of inferring one from the first commit)',
                        default=None)

    return parser.parse_args()


def get_remote_version(branch_to_compare):
    global config
    decoded_file = get_file_contents(config.github_token, BRAVE_CORE_REPO,
                                     'package.json', branch_to_compare)
    json_file = json.loads(decoded_file)
    return json_file['version']


def fancy_print(text):
    print('#' * len(text))
    print(text)
    print('#' * len(text))


def parse_issues_fixed(body):
    try:
        regex = r'((Resolves|Fixes|Fix|Closes|Close|resolves|fixes|fix|closes|close) https:\/\/github\.com\/brave\/brave-browser\/issues\/(\d*))'  # nopep8
        return re.findall(regex, body)
    except Exception as e:
        print(str(e))
        return []


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

    # get all channel branches (starting at master)
    brave_core_version = get_remote_version('master')
    remote_branches = get_remote_channel_branches(brave_core_version)
    top_level_base = 'master'
    issues_fixed = []

    # if starting point is NOT nightly, remove options which aren't desired
    # also, find the branch which should be used for diffs (for cherry-picking)
    if not is_nightly(args.start_from):
        top_level_base = remote_branches[args.start_from]
        try:
            start_index = config.channel_names.index(args.start_from)
            config.channels_to_process = config.channel_names[start_index:]
        except Exception:
            print('[ERROR] specified `start-from` value "' +
                  args.start_from + '" not found in channel list')
            return 1

    # optionally (instead of having a local branch), allow uplifting a specific PR
    # this pulls down the pr locally (in a special branch)
    if args.uplift_using_pr:
        try:
            pr_number = int(args.uplift_using_pr)
            repo = GitHub(config.github_token).repos(BRAVE_CORE_REPO)
            # get enough details from PR to check out locally
            response = repo.pulls(pr_number).get()
            head = response['head']
            local_branch = 'pr' + str(pr_number) + '_' + head['ref']
            head_sha = head['sha']
            top_level_base = response['base']['ref']
            top_level_sha = response['base']['sha']
            merged_at = str(response['merged_at']).strip()
            config.title = str(response['title']).strip()
            issues_fixed = parse_issues_fixed(response['body'])

        except Exception as e:
            print('[ERROR] Error parsing or error returned from API when looking up pull request "' +
                  str(args.uplift_using_pr) + '":\n' + str(e))
            return 1

        # set starting point AHEAD of the PR provided
        config.master_pr_number = pr_number
        if top_level_base == 'master':
            config.channels_to_process = config.channel_names[1:]
        elif len(config.channels_to_process) == 0:
            branch_index = remote_branches.index(top_level_base)
            config.channels_to_process = config.channel_names[branch_index:]

        # if PR was already merged, use the SHA it was PRed against
        if merged_at != 'None' and len(merged_at) > 0:
            print('pr was already merged at ' + merged_at + '; using "' + top_level_sha +
                  '" instead of "' + top_level_base + '"')
            top_level_base = top_level_sha
        else:
            print('[WARNING] Pull request ' + str(pr_number) + ' has not been merged yet.')

        execute(['git', 'fetch', 'origin', 'pull/' +
                args.uplift_using_pr + '/head'])
        # create local branch which matches the contents of the PR
        with scoped_cwd(BRAVE_CORE_ROOT):
            # check if branch exists already
            try:
                branch_sha = execute(
                    ['git', 'rev-parse', '-q', '--verify', local_branch])
            except Exception:
                branch_sha = ''
            if len(branch_sha) > 0:
                # branch exists; reset it
                print('branch "' + local_branch + '" exists; resetting to origin/' + head['ref'] +
                      ' (' + head_sha + ')')
                execute(['git', 'checkout', local_branch])
                execute(['git', 'reset', '--hard', head_sha])
            else:
                # create the branch
                print('creating branch "' + local_branch +
                      '" using origin/' + head['ref'] + ' (' + head_sha + ')')
                execute(['git', 'checkout', '-b', local_branch, head_sha])

    # If title isn't set already, generate one from first commit
    local_branch = get_local_branch_name(BRAVE_CORE_ROOT)
    if not config.title and not args.uplift_using_pr:
        config.title = get_title_from_first_commit(
            BRAVE_CORE_ROOT, top_level_base)

    # Create a branch for each channel
    print('\nCreating branches...')
    fancy_print('NOTE: Commits are being detected by diffing "' +
                local_branch + '" against "' + top_level_base + '"')
    local_branches = {}
    branch = ''
    try:
        for channel in config.channels_to_process:
            branch = create_branch(channel, top_level_base,
                                   remote_branches[channel], local_branch, args)
            local_branches[channel] = branch
            if channel == args.uplift_to:
                break
    except Exception as e:
        print('[ERROR] cherry-pick failed for branch "' +
              branch + '". Please resolve manually:\n' + str(e))
        return 1

    print('\nPushing local branches to remote...')
    push_branches_to_remote(BRAVE_CORE_ROOT, config.branches_to_push,
                            dryrun=config.is_dryrun, token=config.github_token)

    try:
        print('\nCreating the pull requests...')
        for channel in config.channels_to_process:
            submit_pr(
                channel,
                top_level_base,
                remote_branches[channel],
                local_branches[channel],
                issues_fixed)
            if channel == args.uplift_to:
                break
        print('\nDone!')
    except Exception as e:
        print('\n[ERROR] Unhandled error while creating pull request; ' + str(e))
        return 1

    return 0


def is_sha(ref):
    global config
    repo = GitHub(config.github_token).repos(BRAVE_CORE_REPO)
    try:
        repo.git.commits(str(ref)).get()
    except Exception as e:
        response = str(e)
        try:
            # Unsure of original intention, but github.py (on purpose) throws
            # an exception if the response body contains `message`.
            # This catch has the response (in JSON) in exception
            json_response = json.loads(response)
            if json_response['sha'] == ref:
                return True
        except Exception:
            return False
        return False
    return False


def create_branch(channel, top_level_base, remote_base, local_branch, args):
    global config

    if is_nightly(channel):
        return local_branch

    channel_branch = local_branch + '_' + remote_base

    if is_sha(top_level_base):
        compare_from = top_level_base
    else:
        compare_from = 'origin/' + top_level_base

    with scoped_cwd(BRAVE_CORE_ROOT):
        # get SHA for all commits (in order)
        sha_list = execute(['git', 'log', compare_from + '..HEAD',
                           '--pretty=format:%h', '--reverse'])
        sha_list = sha_list.split('\n')
        if len(sha_list) == 0:
            raise Exception('No changes detected!')
        try:
            # check if branch exists already
            try:
                branch_sha = execute(
                    ['git', 'rev-parse', '-q', '--verify', channel_branch])
            except Exception:
                branch_sha = ''

            if len(branch_sha) > 0:
                # branch exists; reset it
                print('(' + channel + ') branch "' + channel_branch +
                      '" exists; resetting to origin/' + remote_base)
                execute(['git', 'checkout', channel_branch])
                execute(['git', 'reset', '--hard', 'origin/' + remote_base])
            else:
                # create the branch
                print('(' + channel + ') creating "' +
                      channel_branch + '" from ' + remote_base)
                execute(['git', 'checkout', remote_base])
                execute(['git', 'pull', 'origin', remote_base])
                execute(['git', 'checkout', '-b', channel_branch])

            # TODO: handle errors thrown by cherry-pick
            for sha in sha_list:
                output = execute(['git', 'cherry-pick', sha]).split('\n')
                print('- picked ' + sha + ' (' + output[0] + ')')

            # squash all commits into one
            # NOTE: master is not squashed. This only runs for uplifts.
            execute(['git', 'reset', '--soft', remote_base])
            squash_message = 'Squash of commits from branch "' + \
                str(local_branch) + '" to ' + channel
            if int(config.master_pr_number) > 0:
                squash_message = 'Uplift of #' + \
                    str(config.master_pr_number) + ' (squashed) to ' + channel
            if args.gpgsign:
                cmdline = ['git', 'commit', '-S', '-m', squash_message]
            else:
                cmdline = ['git', 'commit', '-m', squash_message]
            execute(cmdline)
            squash_hash = execute(['git', 'log', '--pretty="%h"', '-n1'])
            print('- squashed all commits into ' + squash_hash +
                  ' with message: "' + squash_message + '"')

        finally:
            # switch back to original branch
            execute(['git', 'checkout', local_branch])
            execute(['git', 'reset', '--hard', sha_list[-1]])

    config.branches_to_push.append(channel_branch)

    return channel_branch


def get_milestone_for_branch(channel_branch):
    global config
    if not config.milestones:
        config.milestones = get_milestones(
            config.github_token, BRAVE_CORE_REPO)
    for milestone in config.milestones:
        if (milestone['title'].startswith(channel_branch + ' - ') or
           milestone['title'].startswith('Android ' + channel_branch + ' - ')):
            return milestone['number']
    return None


def submit_pr(channel, top_level_base, remote_base,
              local_branch, issues_fixed):
    global config

    try:
        milestone_number = get_milestone_for_branch(remote_base)
        if milestone_number is None:
            print('milestone for "' + remote_base + '" was not found!')
            return 0

        print('(' + channel + ') creating pull request')
        pr_title = config.title or ''
        pr_dst = remote_base
        if is_nightly(channel):
            pr_dst = 'master'

        # add uplift specific details (if needed)
        if is_nightly(channel) or local_branch.startswith(top_level_base):
            pr_body = 'TODO: fill me in\n(created using `npm run pr`)'
        else:
            pr_title += ' (uplift to ' + remote_base + ')'
            pr_body = 'Uplift of #' + str(config.master_pr_number) + '\n'

            if len(issues_fixed) > 0:
                for fixed in issues_fixed:
                    pr_body += (fixed[0] + '\n')

            pr_body += '\nPre-approval checklist: \n'
            pr_body += '- [ ] You have tested your change on Nightly. \n'
            pr_body += '- [ ] This contains text which needs to be translated. \n'
            pr_body += '    - [ ] There are more than 7 days before the release. \n'
            pr_body += '    - [ ] I\'ve notified folks in #l10n on Slack that translations are needed. \n'
            pr_body += '- [ ] The PR milestones match the branch they are landing to. \n\n'

            pr_body += '\nPre-merge checklist: \n'
            pr_body += '- [ ] You have checked CI and the builds, lint, and tests all ' \
                       'pass or are not related to your PR. \n\n'

            pr_body += 'Post-merge checklist: \n'
            pr_body += '- [ ] The associated issue milestone is set to the smallest version ' \
                       'that the changes is landed on.'

        number = create_pull_request(config.github_token, BRAVE_CORE_REPO, pr_title, pr_body,
                                     branch_src=local_branch, branch_dst=pr_dst,
                                     open_in_browser=True, verbose=config.is_verbose, dryrun=config.is_dryrun)

        # store the original PR number so that it can be referenced in uplifts
        if is_nightly(channel) or local_branch.startswith(top_level_base):
            config.master_pr_number = number

        # assign milestone / reviewer(s) / owner(s)
        add_reviewers_to_pull_request(config.github_token, BRAVE_CORE_REPO, number,
                                      team_reviewers=config.team_reviewers,
                                      verbose=config.is_verbose, dryrun=config.is_dryrun)
        set_issue_details(config.github_token, BRAVE_CORE_REPO, number, milestone_number,
                          config.parsed_owners, config.labels,
                          verbose=config.is_verbose, dryrun=config.is_dryrun)
    except Exception as e:
        print('[ERROR] unhandled error occurred:', str(e))
    return 0


if __name__ == '__main__':
    sys.exit(main())

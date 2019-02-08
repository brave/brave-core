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
from lib.config import get_env_var, SOURCE_ROOT, get_raw_version
from lib.util import execute, scoped_cwd
from lib.helpers import *
from lib.github import GitHub


channel_names = channels()
branches_to_push = []


def main():
    print('[INFO] Running pr...')
    args = parse_args()
    print(args)

    # Repo is defined in lib/helpers.py for now
    repo = GitHub(get_env_var('GITHUB_TOKEN')).repos(BRAVE_REPO)

    # get local version + latest version on remote (master)
    # for more info see: http://developer.github.com/v3/repos/contents
    local_version = get_raw_version()
    file = repo.contents("package.json").get()
    decoded_file = base64.b64decode(file['content'])
    json_file = json.loads(decoded_file)
    remote_nightly_version = json_file['version']

    # if they don't match, rebase is needed
    print('local version: ' + local_version + '; remote version: ' + remote_nightly_version)
    if local_version != remote_nightly_version:
        print('[ERROR] Your branch is out of sync; please rebase against master')
        return 1

    # TODO: check if there are unstaged changes. Error out.

    # get the branch name and the first commit subject (used as the title of the pull request)
    with scoped_cwd(SOURCE_ROOT):
        local_branch = execute(['git', 'rev-parse', '--abbrev-ref', 'HEAD']).strip()
        title_list = execute(['git', 'log', 'origin/master..HEAD', '--pretty=format:%s', '--reverse'])
        title_list = title_list.split('\n')
        if len(title_list) == 0:
            print('whoops- no commits?!')
            # TODO: throw exception
        title = title_list[0]

    # mapping of channels to versions
    base = get_channel_bases(remote_nightly_version)

    # uplift to all applicable channels
    for channel in channel_names:
        branch = create_branch(channel, base[channel], local_branch)
        submit_pr(channel, title, base[channel], branch)
        if channel == args.uplift_to:
            break

    # TODO: push any branches to remote
    # TODO: if any failures happen, clean up

    return 0


# put the X in for patch level
def get_branch_name(version):
    version = str(version)
    if version[0] == 'v':
        version = version[1:]
    parts = version.split('.', 3)
    parts[2] = 'x'
    return '.'.join(parts)

# given something like "0.60.x", get previous version ("0.59.x")
def get_previous_version(version):
    version = str(version)
    if version[0] == 'v':
        version = version[1:]

    parts = version.split('.', 3)
    parts[1] = str(int(parts[1]) - 1)
    parts[2] = 'x'
    return '.'.join(parts)


def get_channel_bases(raw_nightly_version):
    nightly_version = get_branch_name(raw_nightly_version)
    dev_version = get_previous_version(nightly_version)
    beta_version = get_previous_version(dev_version)
    release_version = get_previous_version(beta_version)
    return {
        channel_names[0]: nightly_version,
        channel_names[1]: dev_version,
        channel_names[2]: beta_version,
        channel_names[3]: release_version
    }


def create_branch(channel, base, local_branch):
    if channel != channel_names[0]:
        channel_branch = base + '_' + local_branch

        with scoped_cwd(SOURCE_ROOT):
            # get SHA for all commits (in order)
            sha_list = execute(['git', 'log', 'origin/master..HEAD', '--pretty=format:%h', '--reverse'])
            sha_list = sha_list.split('\n')
            try:
                # check if branch exists already
                try:
                    branch_sha = execute(['git', 'rev-parse', '-q', '--verify', channel_branch])
                except:
                    branch_sha = ''

                if len(branch_sha) > 0:
                    # branch exists; reset it
                    print('(' + channel + ') Branch "' + channel_branch + '" exists; resetting to ' + base)
                    execute(['git', 'reset', '--hard', 'origin/' + base])
                else:
                    # create the branch
                    print('(' + channel + ') Creating "' + channel_branch + '" from ' + channel)
                    execute(['git', 'checkout', base])
                    execute(['git', 'checkout', '-b', channel_branch])

                for sha in sha_list:
                    output = execute(['git', 'cherry-pick', sha]).split('\n')
                    print('- picked ' + sha + '(' + output[0] + ')')

            finally:
                # switch back to original branch
                execute(['git', 'checkout', local_branch])
                execute(['git', 'reset', '--hard', sha_list[-1]])

        return channel_branch
    return local_branch


def submit_pr(channel, title, base, branch):
    print('\nCreating pull request for the ' + channel + ' channel...')

    # for more info see: http://developer.github.com/v3/pulls
    try:
        pr_details = {
            'title': title,
            'head': branch,
            'base': base,
            'body': '',
            'maintainer_can_modify': True
        }

        if channel != channel_names[0]:
            pr_details['title'] += ' (uplift to ' + base + ')'
            pr_details['body'] = 'Uplift of X'

        print(str(pr_details) + '\n')

        return 0

    except Exception as e:
        print('whoops:', str(e))

        # pr = repo.create_pull(
        #     "Update from {}".format(full_upstream_name),
        #     wrap_text(r"""
        #         The upstream repository `{}` has some new changes that aren't in this fork.
        #         So, here they are, ready to be merged!
        #         This Pull Request was created programmatically by the
        #         [githubpullrequests](https://github.com/evandrocoan/githubpullrequests).
        #         """.format(full_upstream_name), single_lines=True),
        #     local_branch,
        #     '{}:{}'.format(upstream_user, upstream_branch),
        #     False
        # )

        # pull = self.repo.create_pull("Pull request created by PyGithub",
        # "Body of the pull request", "topic/RewriteWithGeneratedCode", "BeaverSoftware:master", True)
        # self.assertEqual(pull.id, 1436215)

        # Then play with your Github objects
        # successful_resquests += 1
        # log(1, 'Successfully Created:', pr)

        # self.repositories_results['Successfully Created'].append(full_downstream_name)
        # pr.add_to_labels("backstroke")

    # except github.GithubException as error:
    #     error = "%s, %s" % (full_downstream_name, str(error))
    #     log(1, 'Skipping... %s', error)

    #     for reason in self.skip_reasons:
    #         if reason in error:
    #             self.repositories_results[reason].append(full_downstream_name)
    #             break

    #     else:
    #         self.repositories_results['Unknown Reason'].append(error)


def parse_args():
    parser = argparse.ArgumentParser(description='create PRs for all branches given branch against master')
    parser.add_argument('--reviewer',
                        help='comma separated list of reviewers',
                        default=None)
    parser.add_argument('--uplift-to',
                        help='starting at nightly (master), how far back to uplift the changes',
                        default='nightly')

    # TODO: validate channel name!

    return parser.parse_args()


if __name__ == '__main__':
    import sys
    sys.exit(main())

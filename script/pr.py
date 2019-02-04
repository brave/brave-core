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
from lib.config import PLATFORM, DIST_URL, get_target_arch, get_chromedriver_version, \
                       get_env_var, get_zip_name, product_name, project_name, \
                       SOURCE_ROOT, dist_dir, output_dir, get_brave_version, get_raw_version
from lib.util import execute, parse_version, scoped_cwd
from lib.helpers import *

from lib.github import GitHub


def main():
    print('[INFO] Running pr...')
    args = parse_args()
    print(args)

    # Repo is defined in lib/helpers.py for now
    repo = GitHub(get_env_var('GITHUB_TOKEN')).repos(BRAVE_REPO)

    # get local version + latest version on remote (master)
    local_version = get_raw_version()
    file = repo.contents("package.json").get()
    decoded_file = base64.b64decode(file['content'])
    json_file = json.loads(decoded_file)
    remote_version = json_file['version']

    # if they don't match, rebase is needed
    print('local version: ' + local_version + '; remote version: ' + remote_version)
    if local_version != remote_version:
        print('[ERROR] Your branch is out of sync; please rebase against master')
        return 1

    # uplift to all applicable channels
    for channel in channels():
        submit_pr(channel)
        if channel == args.uplift_to:
            break

    return 0


def submit_pr(channel):
    print('Creating pull request for the ' + channel + ' channel...')
    return 0
    try:
        pr = repo.create_pull(
            "Update from {}".format(full_upstream_name),
            wrap_text(r"""
                The upstream repository `{}` has some new changes that aren't in this fork.
                So, here they are, ready to be merged!
                This Pull Request was created programmatically by the
                [githubpullrequests](https://github.com/evandrocoan/githubpullrequests).
                """.format(full_upstream_name), single_lines=True),
            local_branch,
            '{}:{}'.format(upstream_user, upstream_branch),
            False
        )

        # pull = self.repo.create_pull("Pull request created by PyGithub",
        # "Body of the pull request", "topic/RewriteWithGeneratedCode", "BeaverSoftware:master", True)
        # self.assertEqual(pull.id, 1436215)

        # Then play with your Github objects
        successful_resquests += 1
        log(1, 'Successfully Created:', pr)

        self.repositories_results['Successfully Created'].append(full_downstream_name)
        pr.add_to_labels("backstroke")

    except github.GithubException as error:
        error = "%s, %s" % (full_downstream_name, str(error))
        log(1, 'Skipping... %s', error)

        for reason in self.skip_reasons:
            if reason in error:
                self.repositories_results[reason].append(full_downstream_name)
                break

        else:
            self.repositories_results['Unknown Reason'].append(error)


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

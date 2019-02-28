#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import errno
import hashlib
import math
import os
import requests
import re
import shutil
import subprocess
import sys
import json

from io import StringIO
from lib.config import get_env_var
from lib.github import GitHub
from lib.helpers import *
from lib.util import download, execute, tempdir


tag_names = []
releases = {}


def parse_args():
    parser = argparse.ArgumentParser(description='create PRs for all branches given branch against master')
    parser.add_argument('--bad',
                        help='optional version which is known to be bad',
                        default=None)
    parser.add_argument('--good',
                        help='optional version which is known to be good',
                        default=None)
    parser.add_argument('--branch',
                        help='optional branch where the problem is occurring',
                        default=None)

    return parser.parse_args()


def get_releases(repo):
    global releases
    global tag_names

    releases = {}
    tag_names = []

    # get all the releases and index them
    response = repo.releases.get()
    for release in response:
        # skip releases in draft status
        if release['draft']:
            continue
        tag_name = str(release['tag_name'].strip().replace('v', ''))
        tag_names.append(tag_name)
        releases[tag_name] = release

    print('fetched all releases from GitHub (' + str(len(tag_names)) + ' versions found)')


def filter_releases(args):
    global tag_names

    filtered_tag_names = []

    print('filtering out versions which are not relevant')

    # find all unique MINOR versions
    for tag in tag_names:
        version = tag.split('.')
        if len(version) != 3:
            continue

        major_version = str(version[0])
        minor_version = str(version[1])
        branch_version = major_version + '.' + minor_version + '.x'

        if args.branch is not None and branch_version != args.branch:
            print(' - skipping "' + tag + '" (' + branch_version + ' != ' + args.branch + ')')
            continue

        # TODO: skip if there's no DMG/installer available (or platform specific binary)
        #if not get_release_download_url(releases, tag):

        filtered_tag_names.append(tag)

    tag_names = filtered_tag_names


def get_release_asset(version):
    global releases

    release_id = releases[version]['id']
    print('getting installer for  "' + version + '" (release id ' + str(release_id) + ')')

    # find correct asset for platform
    is_mac_os = True
    for asset in releases[version]['assets']:
        if str(asset['name']).endswith('.dmg') and is_mac_os:
            print('- binary found: ' + asset['browser_download_url'])
            return asset

    print('- binary not found')
    return None


def test_version(attempt, tag):
    global tag_names

    # get the OS specific installer
    asset = None
    while len(tag_names) > 0 and not asset:
        print('attempt ' + str(attempt) + '] getting installer for  "' + tag + '" (release id ' + str(releases[tag]['id']) + ')')
        asset = get_release_asset(tag)
        if not asset:
            return False


    download_dir = tempdir('build-bisect_')
    download_path = os.path.join(download_dir, asset['name'])
    print('- downloading to ' + download_path)
    download(tag, asset['browser_download_url'], download_path)


    print('- installing')


    print('- running')
    # execute('')
    # wait for process to return


    answer = raw_input('Did this version work?: y/n\n')
    return answer == 'y'


def get_github_token():
    github_token = get_env_var('GITHUB_TOKEN')
    if len(github_token) == 0:
        result = execute(['npm', 'config', 'get', 'BRAVE_GITHUB_TOKEN']).strip()
        if result == 'undefined':
            raise Exception('`BRAVE_GITHUB_TOKEN` value not found!')
        return result


def find_first_broken_version(args):
    global tag_names

    print('bisecting: ' + str(len(tag_names)) + ' versions to test')

    left_index = 0
    right_index = len(tag_names) - 1

    if left_index >= right_index:
        raise Exception('[ERROR] Not enough versions to perform search')

    attempt_number = 1

    # left should be working
    if not args.good:
        left_tag = tag_names[left_index]
        result = test_version(attempt_number, left_tag)
        attempt_number = attempt_number + 1
        if result == False:
            raise Exception('[ERROR] Version "' + left_tag + '" is expected to work but doesn\'t')

    # right should be NOT working
    if not args.bad:
        right_tag = tag_names[right_index]
        result = test_version(attempt_number, right_tag)
        attempt_number = attempt_number + 1
        if result == True:
            raise Exception('[ERROR] Version "' + right_tag + '" is expected to fail but doesn\'t')

    print('L=' + str(left_index) + ', R=' + str(right_index))

    # perform search
    # TODO: This stops too early. Fix that.
    first_result = None
    while left_index < right_index:
        test_index = int(math.floor((left_index + right_index) / 2))
        test_tag = tag_names[test_index]
        print('L=' + str(left_index) + ', R=' + str(right_index) + ', M=' + str(test_index))
        result = test_version(attempt_number, test_tag)

        if first_result is None:
            first_result = result
        else:
            if result != first_result:
                return test_tag

        if result:
            left_index = test_index + 1
        else:
            right_index = test_index - 1

        attempt_number = attempt_number + 1

    raise Exception('[ERROR] Search performed; no problems found')


def main():
    global tag_names

    args = parse_args()

    github_token = get_github_token()
    repo = GitHub(github_token).repos(BRAVE_REPO)

    get_releases(repo)
    tag_names.sort(key=lambda s: map(int, s.split('.')))

    filter_releases(args)
    first_broken_version = find_first_broken_version(args)
    print('DONE: issue first appeared in "' + first_broken_version + '"')

if __name__ == '__main__':
    import sys
    sys.exit(main())

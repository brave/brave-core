#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import os
import json
import requests
from .config import get_raw_version, get_env_var

BRAVE_REPO = "brave/brave-browser"
BRAVE_CORE_REPO = "brave/brave-core"


def channels():
    return ['nightly', 'dev', 'beta', 'release']


def get_channel_display_name():
    raw = channels()
    d = {
        raw[0]: 'Nightly',
        raw[1]: 'Dev',
        raw[2]: 'Beta',
        raw[3]: 'Release'
    }

    return d[release_channel()]


def call_github_api(url, headers):
    try:
        r = requests.get(url, headers=headers)
    except requests.exceptions.ConnectionError as e:
        print("Error: Received requests.exceptions.ConnectionError, Exiting...")
        exit(1)
    except Exception as e:
        raise Exception(e)

    if r.status_code is 200:
        return r


def get_releases_by_tag(repo, tag_name, include_drafts=False):

    GITHUB_URL = 'https://api.github.com'

    next_request = ""
    headers = {'Accept': 'application/vnd.github+json'}
    release_url = GITHUB_URL + "/repos/brave/brave-browser/releases" + '?access_token=' + \
        get_env_var('GITHUB_TOKEN') + '&page=1&per_page=100'
    r = call_github_api(release_url, headers=headers)
    next_request = ""
    # The GitHub API returns paginated results of 100 items maximum per
    # response. We will loop until there is no next link header returned
    # in the response header. This is documented here:
    # https://developer.github.com/v3/#pagination
    while next_request is not None:
        for item in r.json():
            # print("DEBUG: release: {}".format(item['name']))
            if include_drafts:
                if item['tag_name'] == tag_name:
                    return [item]
            else:
                if item['tag_name'] == tag_name and not item['draft']:
                    return [item]
        if r.links.get("next"):
            next_request = r.links["next"]["url"]
            r = call_github_api(next_request, headers=headers)
        else:
            next_request = None
    return []

def release_channel():
    channel = os.environ['CHANNEL']
    message = ('Error: Please set the $CHANNEL '
               'environment variable, which is your release channel')
    assert channel, message
    return channel


def get_tag():
    return 'v' + get_raw_version() + release_channel()


def release_name():
    return '{0} Channel'.format(get_channel_display_name())


def retry_func(try_func, catch, retries, catch_func=None):
    for count in range(0, retries + 1):
        try:
            ret = try_func(count)
            break
        except catch as e:
            print('[ERROR] Caught exception {}, {} retries left. {}'.format(
                catch, count, e.message))
            if catch_func:
                catch_func(count)
            if count >= retries:
                raise e
    return ret

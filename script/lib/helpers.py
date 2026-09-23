# Copyright (c) 2018 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import print_function
from builtins import range
import os
import sys
import urllib.request
from .config import get_raw_version, get_env_var

BRAVE_REPO = "brave/brave-browser"
BRAVE_CORE_REPO = "brave/brave-core"


def channels():
    return ['nightly', 'beta', 'release']


def get_channel_display_name():
    raw = channels()
    d = {raw[0]: 'Nightly', raw[1]: 'Beta', raw[2]: 'Release'}

    return d[release_channel()]


def get_releases_by_tag(repo, tag_name, include_drafts=False):
    page = 1
    while True:
        releases = repo.releases().get(params={'page': page, 'per_page': 100})
        if not releases:
            break
        for item in releases:
            # print("DEBUG: release: {}".format(item['name']))
            if include_drafts:
                if item['tag_name'] == tag_name:
                    return [item]
            else:
                if item['tag_name'] == tag_name and not item['draft']:
                    return [item]
        page += 1
    return []


def get_release(repo, tag, allow_published_release_updates=False):
    """
    allow_published_release_updates determines whether we will
    allow this process to update only a draft release, or will
    we also allow a published release to be updated.
    """
    release = None
    releases = get_releases_by_tag(repo, tag, include_drafts=True)
    if releases:
        print("[INFO] Found existing release draft")
        if len(releases) > 1:
            raise UserWarning(
                "[INFO] More then one draft with the tag '{}' "
                "found, not sure which one to merge with.".format(tag))
        release = releases[0]
        if not allow_published_release_updates and not release['draft']:
            raise UserWarning("[INFO] Release with tag '{}' is already "
                              "published, aborting.".format(tag))

    return release


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

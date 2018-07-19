#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import os
import json
from .config import get_raw_version

BRAVE_REPO = "brave/brave-browser"

def get_channel_display_name():
  d = {'beta': 'Beta', 'canary': 'Canary', 'dev': 'Developer', 'release': 'Release'}
  return d[release_channel()]

def get_releases_by_tag(repo, tag_name, include_drafts=False):
  if include_drafts:
    return [r for r in repo.releases.get() if r['tag_name'] == tag_name]
  else:
    
    return [r for r in repo.releases.get() if r['tag_name'] == tag_name and not r['draft']]

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

def get_releases_by_tag(repo, tag_name, include_drafts=False):
  if include_drafts:
    return [r for r in repo.releases.get() if r['tag_name'] == tag_name]
  else:
    return [r for r in repo.releases.get() if r['tag_name'] == tag_name and not r['draft']]

def retry_func(try_func, catch, retries, catch_func=None):
  for count in range(0, retries + 1):
    try:
      ret = try_func(count)
      break
    except catch as e:
      print('[ERROR] Caught exception {}, {} retries left. {}'.format(catch, count, e.message))
      if catch_func:
        catch_func(count)
      if count >= retries:
        raise e
  return ret

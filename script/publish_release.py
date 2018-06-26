#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import json
import os
import sys

from lib.github import GitHub
from lib.config import PLATFORM, DIST_URL, get_target_arch, get_chromedriver_version, \
                       get_env_var, s3_config, get_zip_name, product_name, project_name, \
                       SOURCE_ROOT, dist_dir, output_dir, get_brave_version
from lib.helpers import *
import requests

RELEASE_NAME = 'Dev Channel Beta'

def main():
  repo = GitHub(get_env_var('GITHUB_TOKEN')).repos(BRAVE_REPO)

  release = get_draft(repo, get_brave_version())
  commit_tag = get_commit_tag(get_version())

  print("[INFO] Releasing {}".format(release['tag_name']))
  publish_release(repo, release['id'], get_tag(), commit_tag)

def get_commit_tag(version):
  parts = get_version().split('.', 3)
  if (len(parts) == 3):
    parts[2] = 'x'
    return '.'.join(parts)
  else:
    raise(UserWarning("[ERROR] Invalid version name '%s'", get_version()))

def get_draft(repo, tag):
  releases = get_releases_by_tag(repo, tag, include_drafts=True)
  if not releases:
    raise(UserWarning("[ERROR]: No draft with tag '{}' found, may need to run the ./tools/upload.py script first".format(tag)))
  elif len(releases) > 1 or not releases[0]['draft']:
    raise(UserWarning("[ERROR]: Release with tag {} already exists".format(tag)))
  return releases[0]

def publish_release(repo, release_id, tag, commit_tag):
  data = dict(draft=False, prerelease=True, tag_name=tag, target_commitish=commit_tag)

  try:
    repo.releases(release_id).patch(data=data)
  except Exception as e:
    if any([err.get("field") == "tag_name"  for err in json.loads(e.message)['errors']]):
      print("[ERROR] Please make sure tag_name exists and is pushed to github")
      raise

if __name__ == '__main__':
  import sys
  sys.exit(main())

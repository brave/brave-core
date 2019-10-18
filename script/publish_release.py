#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import json
import logging
import os
import sys
from lib.github import GitHub
from lib.config import (PLATFORM, DIST_URL, get_target_arch,
                        get_env_var,
                        s3_config, get_zip_name, product_name,
                        project_name, SOURCE_ROOT, dist_dir,
                        output_dir, get_brave_version,
                        get_raw_version)
from lib.helpers import *
import requests

from argparse import RawTextHelpFormatter


def main():

    args = parse_args()

    if args.debug:
        logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)
        logging.debug('prerelease: {}'.format(args.prerelease))

    if not os.environ.get('npm_config_brave_version'):
        message = ('Error: Please set the $npm_config_brave_version'
                   'environment variable')
        exit(message)

    BRAVE_VERSION = os.environ.get('npm_config_brave_version')

    repo = GitHub(get_env_var('GITHUB_TOKEN')).repos(BRAVE_REPO)

    tag = get_brave_version()
    logging.debug("Tag: {}".format(tag))

    # If we are publishing a prerelease, the release can only be in draft mode. If we
    # are publishing a full release, it is allowed to already be a published release.
    if args.prerelease:
        release = get_draft(repo, tag)
    else:
        release = get_release(repo, tag, allow_published_release_updates=True)

    tag_name = release['tag_name']
    logging.debug("release[id]: {}".format(release['id']))

    logging.info("Releasing {}".format(tag_name))
    publish_release(repo, release['id'], tag_name, args.prerelease, logging)


def get_draft(repo, tag):
    releases = get_releases_by_tag(repo, tag, include_drafts=True)
    if not releases:
        raise(UserWarning("[ERROR]: No draft with tag '{}' found, may need "
                          "to run the ./script/upload.py script first"
                          .format(tag)))
    elif len(releases) > 1 or not releases[0]['draft']:
        raise(UserWarning("[ERROR]: Release with tag {} already exists"
                          .format(tag)))
    return releases[0]


def publish_release(repo, release_id, tag, prerelease, logging):
    data = dict(prerelease=prerelease, draft=False, tag_name=tag)

    try:
        repo.releases(release_id).patch(data=data)
    except Exception as e:
        if any([err.get("field") == "tag_name" for
                err in json.loads(e.message)['errors']]):
            logging.error("[ERROR] Please make sure tag_name exists and is pushed "
                          "to github")
            raise


def parse_args():
    desc = "Publish GitHub draft release" \
        "\n\nRequires the following ENVIRONMENT VARIABLES be set:" \
        "\n\nCHANNEL: The Brave channel, i.e. \'nightly\', \'dev\', \'beta\', \'release\'"

    parser = argparse.ArgumentParser(
        description=desc, formatter_class=RawTextHelpFormatter)
    parser.add_argument('-d', '--debug', action='store_true',
                        help='Print debug statements')
    parser.add_argument('-p', '--prerelease', help='Publish the release as a `prerelease` '
                        '(Default: False)', action='store_true')
    return parser.parse_args()


if __name__ == '__main__':
    import sys
    sys.exit(main())

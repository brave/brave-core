#!/usr/bin/env python3
import argparse
import logging
import re
import sys

from argparse import RawTextHelpFormatter

from lib.changelog import *
from lib.config import (PLATFORM, get_env_var)
from lib.github import GitHub
from lib.helpers import *


def main():

    """
    Download the brave-browser/CHANGELOG.md file, parse it and
    convert to markdown, then update the release notes for the
    release specified.

    """

    args = parse_args()

    if args.debug:
        logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)

    changelog_url = args.url

    tag = args.tag

    if not re.match(r'^refs/tags/', tag) and not re.match(r'^v', tag):
        logging.error(" Tag prefix must contain {} or {}".format("\"refs/tags/\"", "\"v\""))
        exit(1)

    match = re.match(r'^refs/tags/(.*)$', tag)
    if match:
        tag = match.group(1)

    match = re.match(r'^v(.*)$', tag)
    if match:
        version = match.group(1)

    logging.debug("CHANGELOG_URL: {}".format(changelog_url))
    logging.debug("TAG: {}".format(tag))
    logging.debug("VERSION: {}".format(version))

    changelog_txt = download_from_url(args, logging, changelog_url)

    tag_changelog_txt = render_markdown(changelog_txt, version, logging)

    # BRAVE_REPO is defined in lib/helpers.py
    repo = GitHub(get_env_var('GITHUB_TOKEN')).repos(BRAVE_REPO)
    release = get_release(repo, tag, allow_published_release_updates=True)

    logging.debug("Release body before update: \n\'{}\'".format(release['body']))

    logging.info("Merging original release body with changelog")
    new_body = release['body'] + '\n\n### Release Notes' + '\n\n' + \
        tag_changelog_txt
    logging.debug("release body is now: \n\'{}\'".format(new_body))

    data = dict(tag_name=tag, name=release['name'], body=new_body)
    id = release['id']
    logging.debug("Updating release with id: {}".format(id))
    release = retry_func(
        lambda run: repo.releases.__call__(f'{id}').patch(data=data),
        catch=requests.exceptions.ConnectionError, retries=3
    )
    logging.debug("Release body after update: \n\'{}\'".format(release['body']))


def parse_args():
    desc = "Parse Brave Browser changelog and add markdown to release notes for tag"

    parser = argparse.ArgumentParser(
        description=desc, formatter_class=RawTextHelpFormatter)
    parser.add_argument('-d', '--debug', action='store_true',
                        help='Print debug statements')
    parser.add_argument('-t', '--tag',
                        help='Brave version tag (allowed format: "v0.60.45" or "refs/tags/v0.60.45")', required=True)
    parser.add_argument('-u', '--url', help='URL for Brave Browser raw markdown file (required)', required=True)
    return parser.parse_args()


if __name__ == '__main__':
    sys.exit(main())

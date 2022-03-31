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

if os.environ.get('DEBUG_HTTP_HEADERS') == 'true':
    try:
        from http.client import HTTPConnection  # python3
    except ImportError:
        from httplib import HTTPConnection  # python2


def main():
    """
    Download the brave-browser/CHANGELOG.md file, parse it and
    convert to markdown, then update the release notes for the
    release specified.

    """

    # Enable urllib3 debugging output
    if os.environ.get('DEBUG_HTTP_HEADERS') == 'true':
        logging.basicConfig(level=logging.DEBUG)
        logging.getLogger("urllib3").setLevel(logging.DEBUG)
        logging.debug(
            "DEBUG_HTTP_HEADERS env var is enabled, logging HTTP headers")
        debug_requests_on()

    args = parse_args()

    if args.debug:
        logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)

    changelog_url = args.url

    tag = args.tag

    if not re.match(r'^refs/tags/', tag) and not re.match(r'^v', tag):
        logging.error(" Tag prefix must contain {} or {}".format(
            "\"refs/tags/\"", "\"v\""))
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

    logging.debug(
        "Release body before update: \n\'{}\'".format(release['body']))

    logging.info("Merging original release body with changelog")
    new_body = release['body'] + '\n\n' + tag_changelog_txt
    logging.debug("release body is now: \n\'{}\'".format(new_body))

    data = dict(tag_name=tag, name=release['name'], body=new_body)
    id = release['id']
    logging.debug("Updating release with id: {}".format(id))
    release = retry_func(
        lambda run: repo.releases.__call__(f'{id}').patch(data=data),
        catch=requests.exceptions.ConnectionError, retries=3
    )
    logging.debug(
        "Release body after update: \n\'{}\'".format(release['body']))


def debug_requests_on():
    '''Switches on logging of the requests module.'''
    HTTPConnection.debuglevel = 1

    logging.basicConfig()
    logging.getLogger().setLevel(logging.DEBUG)
    requests_log = logging.getLogger("requests.packages.urllib3")
    requests_log.setLevel(logging.DEBUG)
    requests_log.propagate = True


def debug_requests_off():
    '''Switches off logging of the requests module, might be some side-effects'''
    HTTPConnection.debuglevel = 0

    root_logger = logging.getLogger()
    root_logger.setLevel(logging.WARNING)
    root_logger.handlers = []
    requests_log = logging.getLogger("requests.packages.urllib3")
    requests_log.setLevel(logging.WARNING)


def parse_args():
    desc = "Parse Brave Browser changelog and add markdown to release notes for tag" \
        "\n\nRequires the following ENVIRONMENT VARIABLES be set:" \
        "\n\nBRAVE_GITHUB_TOKEN: Github token to update draft release if not published yet. "

    parser = argparse.ArgumentParser(
        description=desc, formatter_class=RawTextHelpFormatter)
    parser.add_argument('-d', '--debug', action='store_true',
                        help='Print debug statements')
    parser.add_argument('-t', '--tag',
                        help='Brave version tag (allowed format: "v1.5.45" or "refs/tags/v1.5.45") (required)',
                        required=True)
    parser.add_argument(
        '-u', '--url', help='URL for Brave Browser raw markdown file (required)', required=True)
    return parser.parse_args()


if __name__ == '__main__':
    sys.exit(main())

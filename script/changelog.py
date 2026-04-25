#!/usr/bin/env python3
import argparse
import logging
import re
import sys

from argparse import RawTextHelpFormatter

from lib.changelog import *


def main():
    """
    Download the brave-browser/CHANGELOG.md file, parse it, and
    return either markdown or html for a particular Brave tag.

    Example:
    python script/changelog.py -t refs/tags/v1.5.51 \
        -u https://raw.githubusercontent.com/brave/brave-browser/master/CHANGELOG.md -o markdown

    ## [1.5.51](https://github.com/brave/brave-browser/releases/tag/v1.5.51)

    - Added new setting that allows Brave Rewards icon in the URL to be hidden if Rewards \
        is inactive. ([#2975](https://github.com/brave/brave-browser/issues/2975))

    """

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

    if args.output == 'markdown':
        print(render_markdown(changelog_txt, version, logging))
    elif args.output == 'html':
        print(render_html(changelog_txt, version, logging))


def parse_args():
    desc = "Parse Brave Browser changelog and return changes for a tag"

    parser = argparse.ArgumentParser(
        description=desc, formatter_class=RawTextHelpFormatter)
    parser.add_argument('-d', '--debug', action='store_true',
                        help='Print debug statements')
    parser.add_argument(
        '-o', '--output', help='Output format: markdown or html (required)', required=True)
    parser.add_argument('-t', '--tag',
                        help='Brave version tag (allowed format: "v1.5.45" or "refs/tags/v1.5.45") (required)',
                        required=True)
    parser.add_argument(
        '-u', '--url', help='URL for Brave Browser raw markdown file (required)', required=True)
    return parser.parse_args()


if __name__ == '__main__':
    sys.exit(main())

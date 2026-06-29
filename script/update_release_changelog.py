#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import logging
import os
import re
import sys

import requests
from argparse import RawTextHelpFormatter

from lib.changelog import download_from_url
from lib.github import GitHub
from lib.helpers import BRAVE_REPO, get_release, retry_func

if os.environ.get('DEBUG_HTTP_HEADERS') == 'true':
    try:
        from http.client import HTTPConnection  # python3
    except ImportError:
        from httplib import HTTPConnection  # python2


DEFAULT_SECTION_TITLE = 'Release Notes'

H1_SECTION_RE = re.compile(
    r'^# (.+?)\s*\n(.*?)(?=^\# |\Z)',
    flags=re.MULTILINE | re.DOTALL,
)


def parse_h1_sections(body):
    """Split release body into preamble text and ordered H1 sections."""
    body = (body or '').rstrip()
    if not body:
        return '', []

    matches = list(H1_SECTION_RE.finditer(body))
    if not matches:
        return body, []

    preamble = body[:matches[0].start()].rstrip()
    sections = [(match.group(1), match.group(2).rstrip()) for match in matches]
    return preamble, sections


def assemble_body(preamble, sections):
    """Rebuild release body from preamble and H1 sections."""
    parts = []
    if preamble:
        parts.append(preamble)
    for title, content in sections:
        parts.append('# {}\n\n{}'.format(title, content.rstrip()))
    return '\n\n'.join(parts)


def normalize_section_title(title):
    """Normalize an H1 title; ignore case and all whitespace when matching."""
    return re.sub(r'\s+', '', title.strip().lower())


def merge_release_changelog_section(body, section_title, section_content,
                                    logger):
    """Insert changelog section after removing all prior matching sections."""
    normalized_title = section_title.strip()
    remove_titles = {normalize_section_title(section_title)}
    preamble, sections = parse_h1_sections(body)
    kept = []
    insert_index = None

    for title, content in sections:
        if normalize_section_title(title) in remove_titles:
            logger.info('Removing existing "%s" section: %s', normalized_title,
                        title.strip())
            if insert_index is None:
                insert_index = len(kept)
            continue
        kept.append((title, content))

    new_section = (normalized_title, section_content.rstrip())
    if insert_index is None:
        kept.append(new_section)
    else:
        kept.insert(insert_index, new_section)

    return assemble_body(preamble, kept)


TAG_FORMAT_HELP = ('Tag must be "vX.Y.Z" or "refs/tags/vX.Y.Z" '
                   '(example: v1.5.45 or refs/tags/v1.5.45)')


def normalize_tag(raw_tag):
    """Return (tag, version) from vX.Y.Z or refs/tags/vX.Y.Z."""
    match = re.match(r'^(?:refs/tags/)?v(\d+\.\d+\.\d+)$', raw_tag)
    if not match:
        logging.error('%s: %s', TAG_FORMAT_HELP, raw_tag)
        sys.exit(1)

    version = match.group(1)
    return 'v' + version, version


def version_header_pattern(version):
    """Match version on an H2 header without digit/dot prefix overlap."""
    return r'(?<![\d.])' + re.escape(version) + r'(?![\d.])'


def extract_changelog_section(changelog_txt, version):
    """Return release notes body for version from a CHANGELOG.md excerpt."""
    header_pattern = (r'^## [^\n]*' + version_header_pattern(version) +
                      r'[^\n]*\n+')
    rn_regex = re.compile(header_pattern + r'(.*?)(?:\n+^##\s|\Z)',
                          flags=re.DOTALL | re.MULTILINE)
    match = rn_regex.search(changelog_txt)
    if not match:
        return None
    return match.group(1)


def main():
    """
    Download a Brave Browser or Brave Origin CHANGELOG.md file, extract the
    release notes for the requested tag, and merge them into the matching
    GitHub release body.

    The excerpt is inserted under an H1 heading (# <title>) from
    --section-title (default: "Release Notes"). Existing H1 sections with
    the same title (case-insensitive; all whitespace ignored) are replaced
    rather than duplicated. Other release-body sections are left unchanged.

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

    tag, version = normalize_tag(args.tag)

    logging.debug("CHANGELOG_URL: %s", changelog_url)
    logging.debug("TAG: %s", tag)
    logging.debug("VERSION: %s", version)

    changelog_txt = download_from_url(args, logging, changelog_url)

    changelog_section = extract_changelog_section(changelog_txt, version)
    if not changelog_section:
        logging.error("Unable to locate release notes!")
        sys.exit(1)

    # BRAVE_REPO is defined in lib/helpers.py
    repo = GitHub(os.environ.get('GITHUB_TOKEN')).repos(BRAVE_REPO)
    release = get_release(repo, tag, allow_published_release_updates=True)
    if release is None:
        logging.error("Release not found for tag: %s", tag)
        sys.exit(1)

    logging.debug("Release body before update: \n'%s'", release['body'])

    logging.info("Merging original release body with changelog")
    new_body = merge_release_changelog_section(release['body'],
                                               args.section_title,
                                               changelog_section, logging)
    logging.debug("release body is now: \n'%s'", new_body)

    data = dict(tag_name=tag, name=release['name'], body=new_body)
    release_id = release['id']
    logging.debug("Updating release with id: %s", release_id)
    release = retry_func(lambda _attempt: repo.releases.__call__(
        f'{release_id}').patch(data=data),
                         catch=requests.exceptions.ConnectionError,
                         retries=3)
    logging.debug("Release body after update: \n'%s'", release['body'])


def debug_requests_on():
    '''Switches on logging of the requests module.'''
    HTTPConnection.debuglevel = 1

    logging.basicConfig()
    logging.getLogger().setLevel(logging.DEBUG)
    requests_log = logging.getLogger("requests.packages.urllib3")
    requests_log.setLevel(logging.DEBUG)
    requests_log.propagate = True


def debug_requests_off():
    '''Switches off logging of the requests module; might have side effects.'''
    HTTPConnection.debuglevel = 0

    root_logger = logging.getLogger()
    root_logger.setLevel(logging.WARNING)
    root_logger.handlers = []
    requests_log = logging.getLogger("requests.packages.urllib3")
    requests_log.setLevel(logging.WARNING)


def parse_args():
    desc = ("Parse Brave Browser or Brave Origin changelog and add markdown "
            "to release notes for tag\n\nRequires the following ENVIRONMENT "
            "VARIABLES be set:\n\nGITHUB_TOKEN: GitHub token with permission "
            "to update the release body (draft or published). ")

    parser = argparse.ArgumentParser(
        description=desc, formatter_class=RawTextHelpFormatter)
    parser.add_argument('-d', '--debug', action='store_true',
                        help='Print debug statements')
    parser.add_argument(
        '-t',
        '--tag',
        help=('Brave version tag (allowed format: "v1.5.45" or '
              '"refs/tags/v1.5.45") (required)'),
        required=True)
    parser.add_argument(
        '-u',
        '--url',
        help='URL for Brave Browser or Brave Origin CHANGELOG.md (required)',
        required=True)
    parser.add_argument(
        '-s',
        '--section-title',
        default=DEFAULT_SECTION_TITLE,
        help=('H1 section title for the inserted release notes '
              '(written as "# <title>"; replaces existing H1 '
              'sections with the same title; '
              'default: "%(default)s")'))
    return parser.parse_args()


if __name__ == '__main__':
    sys.exit(main())

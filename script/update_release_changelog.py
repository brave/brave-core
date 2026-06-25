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


def merge_release_changelog_section(body, section_title, section_content, logging):
    """Insert changelog section after removing all prior matching sections."""
    remove_titles = {section_title.lower()}
    preamble, sections = parse_h1_sections(body)
    kept = []
    insert_index = None

    for title, content in sections:
        if title.lower() in remove_titles:
            logging.info("Removing existing release notes section: %s", title)
            if insert_index is None:
                insert_index = len(kept)
            continue
        kept.append((title, content))

    new_section = (section_title, section_content.rstrip())
    if insert_index is None:
        kept.append(new_section)
    else:
        kept.insert(insert_index, new_section)

    return assemble_body(preamble, kept)


def main():
    """
    Download the brave-browser/CHANGELOG.md file, parse it and
    convert to markdown, then update the release notes for the
    release specified.

    The changelog excerpt is merged under a markdown heading given by
    --section-title (default: "Release Notes"). Before inserting, every
    existing section with the same heading (case-insensitive), including repeated
    copies, is removed. Other sections (install instructions, other platform notes)
    are kept.

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

    rn_regex = re.compile(r'^## .*?' + version.replace(".", "\\.") +
                          r'.*?$\n+(.*?)\n+^##\s',
                          flags=re.DOTALL | re.MULTILINE)
    match = rn_regex.search(changelog_txt)
    if not match:
        logging.error("Unable to locate release notes!")
        exit(1)

    # BRAVE_REPO is defined in lib/helpers.py
    repo = GitHub(os.environ.get('GITHUB_TOKEN')).repos(BRAVE_REPO)
    release = get_release(repo, tag, allow_published_release_updates=True)

    logging.debug(
        "Release body before update: \n\'{}\'".format(release['body']))

    logging.info("Merging original release body with changelog")
    new_body = merge_release_changelog_section(
        release['body'], args.section_title, match.group(1), logging)
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
        "\n\nGITHUB_TOKEN: Github token to update draft release if not published yet. "

    parser = argparse.ArgumentParser(
        description=desc, formatter_class=RawTextHelpFormatter)
    parser.add_argument('-d', '--debug', action='store_true',
                        help='Print debug statements')
    parser.add_argument('-t', '--tag',
                        help='Brave version tag (allowed format: "v1.5.45" or "refs/tags/v1.5.45") (required)',
                        required=True)
    parser.add_argument(
        '-u', '--url', help='URL for Brave Browser raw markdown file (required)', required=True)
    parser.add_argument(
        '-s', '--section-title', default=DEFAULT_SECTION_TITLE,
        help='Markdown section heading for release notes (default: "%(default)s")')
    return parser.parse_args()


if __name__ == '__main__':
    sys.exit(main())

#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import logging
import os
import re
import sys
import requests

from argparse import RawTextHelpFormatter
from lib.config import get_brave_version, get_raw_version, get_env_var
from lib.connect import post, get, post_with_file
from lib.github import GitHub
from lib.helpers import *
from lib.util import get_host_arch, omaha_channel
from lib.omaha import get_app_info, get_base64_authorization, get_channel_id, get_omaha_version_id, \
    get_upload_version, get_event_id, get_channel_ids_from_omaha_server, sign_update_sparkle

# TODO:
# 1. write tests
# 2. create other apps

# API DOCUMENTATION
# created id jenkins-upload account(PW IN 1password) on the `updates-panel-dev` omaha server
#   to generate base64 string on commandline use 'echo -n userid:password" | base64'
# THE jenkins-upload USER CAN ONLY ADD OMAHA OR SPARKLE VERSION, CANNOT DELETE OR MODIFY EXISTING VERSIONS
# EXAMPLE CURL
# curl -L -D- -X GET -H "Authorization: Basic BASE64USERIDANDPASSWORD" -H "Content-Type: application/json" \
#  "https://OMAHA-HOSTNAME/api/omaha/version"
# https://crystalnix.github.io/omaha-server/#header-supplying-basic-auth-headers
# https://crystalnix.github.io/omaha-server/#omaha-version-version-list-post
# https://crystalnix.github.io/omaha-server/#sparkle-version-version-list-post

# Example of using API to communicate with Sparkle:
# https://gist.github.com/yurtaev/294a5fbd78016e5d7456


def download_from_github(args, logging):
    file_list = []

    # BRAVE_REPO defined in helpers.py
    repo = GitHub(get_env_var('GITHUB_TOKEN')).repos(BRAVE_REPO)

    if args.tag:
        tag_name = args.tag
    else:
        tag_name = get_brave_version()

    release = None
    releases = get_releases_by_tag(repo, tag_name, include_drafts=True)
    if releases:
        if len(releases) > 1:
            exit("Error: More than 1 release exists with the tag: \'{}\'".format(tag_name))
        release = releases[0]
    else:
        exit("Error: Did not get the release \'{}\' from Github.".format(tag_name))

    found_assets_in_github_release = {}

    for asset in release['assets']:
        if re.match(r'.*\.dmg$', asset['name']):
            if args.uploaded:
                if not args.platform:
                    args.platform = []
                args.platform.append('darwin')
            found_assets_in_github_release['darwin'] = {}
            found_assets_in_github_release['darwin']['name'] = asset['name']
            found_assets_in_github_release['darwin']['url'] = asset['url']
        elif re.match(r'brave_installer-ia32\.exe$', asset['name']):
            if args.uploaded:
                if not args.platform:
                    args.platform = []
                args.platform.append('win32')
            found_assets_in_github_release['win32'] = {}
            found_assets_in_github_release['win32']['name'] = asset['name']
            found_assets_in_github_release['win32']['url'] = asset['url']
        elif re.match(r'brave_installer-x64\.exe$', asset['name']):
            if args.uploaded:
                if not args.platform:
                    args.platform = []
                args.platform.append('win64')
            found_assets_in_github_release['win64'] = {}
            found_assets_in_github_release['win64']['name'] = asset['name']
            found_assets_in_github_release['win64']['url'] = asset['url']

    logging.debug("Found assets in github release: {}".format(
                  found_assets_in_github_release))

    for requested_platform in args.platform:
        logging.debug("Verifying platform \'{}\' exists in GitHub release".
                      format(requested_platform))
        if requested_platform not in found_assets_in_github_release.keys():
            logging.error("Platform \'{}\' does not exist in GitHub release".
                          format(requested_platform))
            exit(1)

    for platform in args.platform:
        if args.debug:
            logging.debug("GitHub asset_url: {}".format(
                          found_assets_in_github_release[platform]['url'] + '/'
                          + found_assets_in_github_release[platform]['name']))

        # Instantiate new requests session, versus reusing the repo session above.
        # Headers was likely being reused in that session, and not allowing us
        # to set the Accept header to the below.
        headers = {'Accept': 'application/octet-stream',
                   'Authorization': 'token ' + get_env_var('GITHUB_TOKEN')}

        asset_url = found_assets_in_github_release[platform]['url']

        if args.debug:
            # disable urllib3 logging for this session to avoid showing
            # access_token in logs
            logging.getLogger("urllib3").setLevel(logging.WARNING)

        r = requests.get(asset_url, headers=headers, stream=True)

        if args.debug:
            logging.getLogger("urllib3").setLevel(logging.DEBUG)

        logging.debug("Writing GitHub download to file: {}".format(
                      found_assets_in_github_release[platform]['name']))

        with open(found_assets_in_github_release[platform]['name'], 'wb') as f:
            for chunk in r.iter_content(chunk_size=1024):
                if chunk:
                    f.write(chunk)

        logging.debug(
            "Requests Response status_code: {}".format(r.status_code))
        if r.status_code == 200:
            file_list.append('./' + found_assets_in_github_release[platform]['name'])
        else:
            logging.debug(
                "Requests Response status_code != 200: {}".format(r.status_code))

    if len(file_list) < len(args.platform):
        for item in args.platform:
            logging.error(
                "Cannot get requested file from Github! {}".format(found_assets_in_github_release[item]['name']))
        remove_github_downloaded_files(file_list, logging)
        exit(1)

    return file_list


def remove_github_downloaded_files(file_list, logging):
    for source_file in file_list:
        try:
            os.remove(source_file)
            logging.debug("Removed file: {}".format(source_file))
        except OSError:
            raise


def post_action(host, version, action, headers, args):
    url = 'https://' + host + '/api/action/'
    params = {
        "version": version,
        "event": get_event_id(action)
    }

    if release_channel() in ['nightly']:
        params['arguments'] = "--chrome-sxs"
    elif release_channel() not in ['release']:
        params['arguments'] = "--chrome-" + release_channel()

    if args.debug:
        logging.debug("params: ")
        for item in params:
            logging.debug('{}: {}'.format(item, params[item]))

    response = post(url, params, headers)

    if response.status_code != 201:
        logging.error("ERROR: Action not created! response.status_code : {}".format(
            response.status_code))
        exit(1)


def parse_args():
    desc = "Upload Windows/Mac install files to Omaha server" \
           "\n\nRequires the following ENVIRONMENT VARIABLES be set:" \
           "\n\nCHANNEL: The Brave channel, i.e. \'nightly\', \'dev\', \'beta\', \'release\'" \
           "\nOMAHA_HOST: The FQDN hostname of the Omaha server to upload to. (without \'https:\\\\' prefix)" \
           "\nOMAHA_USER: The UserID to use to login to the Omaha server." \
           "\nOMAHA_PASS: The Password to login to the Omaha server." \
           "\nDSA_PRIVATE_PEM: The Private DSA pem file used to sign the Mac DMG file." \
           "\nBRAVE_GITHUB_TOKEN: Github token to download from a draft release if not published yet. " \
           "(ONLY REQUIRED IF --github)" \
           "\nnpm_config_brave_version: Chromium version (only if not in brave-core directory with brave-browser" \
           " in the parent dir)"
    parser = argparse.ArgumentParser(
        description=desc, formatter_class=RawTextHelpFormatter)
    parser.add_argument('--version', help='full brave version to upload')
    parser.add_argument('--previous', help='previous version')
    parser.add_argument('--platform', help='platforms (spaced)', nargs='*', choices=['win32', 'win64', 'darwin'])
    parser.add_argument('--internal', help='upload to internal test channels', action='store_true')
    parser.add_argument('--full', help='upload to "-full" channels', action='store_true')
    parser.add_argument('--file', help='installer file to upload (cannot be combined with --github)')
    parser.add_argument('--tag', help='GitHub version tag to upload')
    parser.add_argument('--github', help='download from GitHub (cannot be combined with --file)', action='store_true')
    parser.add_argument('--uploaded', help='upload all the platforms from the GitHub release', action='store_true')
    parser.add_argument('--debug', help='debug', action='store_true')
    return parser.parse_args()


def main():
    args = parse_args()

    if args.debug:
        logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)
        logging.debug('brave_version: {}'.format(get_upload_version()))

    if args.version and args.previous:
        if args.version == args.previous:
            exit("Error: version and previous have to be different")

    if args.uploaded and args.platform:
        exit("Error: --platform and --uploaded are mutually exclusive, only one allowed")

    # Default to requiring all 3 platforms
    if not args.uploaded and not args.platform:
        args.platform = ['win32', 'win64', 'darwin']

    if args.debug and args.platform:
        logging.debug("args.platform: {}".format(args.platform))

    if args.file and args.github:
        exit("Error: --file and --github are mutually exclusive, only one allowed")

    if not os.environ.get('OMAHA_PASS') or not os.environ.get('OMAHA_USER'):
        message = ('Error: Please set the $OMAHA_USER, $OMAHA_PASS and $OMAHA_HOST environment variables')
        exit(message)

    if args.github:
        file_list = download_from_github(args, logging)
    else:
        file_list = [args.file]

    for source_file in file_list:
        app_info = {}

        if args.debug:
            logging.debug("source_file: {}".format(source_file))

        if args.github:
            if re.match(r'.*\.dmg$', source_file):
                app_info['platform'] = 'darwin'
                app_info['arch'] = 'x64'
            elif re.match(r'.*brave_installer-ia32\.exe$', source_file):
                app_info['platform'] = 'win32'
                app_info['arch'] = 'ia32'
            elif re.match(r'.*brave_installer-x64\.exe$', source_file):
                app_info['platform'] = 'win32'
                app_info['arch'] = 'x64'

        app_info = get_app_info(app_info, args)
        app_info['omahahost'] = os.environ.get('OMAHA_HOST')
        app_info['auth'] = get_base64_authorization(os.environ.get('OMAHA_USER'), os.environ.get('OMAHA_PASS'))
        app_info['headers'] = headers = {
            'Authorization': 'Basic %s' % app_info['auth']
        }

        if app_info['previous']:
            app_info['version_url'] = '/api/deltaupdate/'
        else:
            if app_info['platform'] in 'darwin':
                app_info['version_url'] = '/api/sparkle/version/'
                if not os.environ.get('DSA_PRIVATE_PEM'):
                    exit('Error: Please set the $DSA_PRIVATE_PEM environment variable')
            elif app_info['platform'] in 'win32':
                app_info['version_url'] = '/api/omaha/version/'

        app_info['version_post_url'] = 'https://' + app_info['omahahost'] + app_info['version_url']
        app_info['size'] = os.path.getsize(source_file)

        channel = omaha_channel(app_info['platform'], app_info['arch'], app_info['internal'], app_info['full'])
        channel_id = get_channel_id(channel, app_info['omahahost'], app_info['headers'], logging)

        if args.debug:
            for item in app_info:
                if item in 'auth':
                    logging.debug('{}: {}'.format(item, "NOTAREALPASSWORD"))
                elif item in 'headers':
                    logging.debug('{}: {}'.format(
                        item, "{'Authorization': 'Basic NOTAREALPASSWORD'}"))
                else:
                    logging.debug('{}: {}'.format(item, app_info[item]))
            logging.debug("omaha_channel: {}".format(channel))
            logging.debug("omaha_channel_id: {}".format(channel_id))
            logging.debug("URL: {}".format(app_info['version_post_url']))
            logging.debug("file_list: {}".format(file_list))

        with open(source_file, 'rb') as f:
            files = {'file': f}

            if not app_info['previous']:
                params = {
                    'app': app_info['appguid'],
                    'channel': channel_id,
                    'version': app_info['version'],
                    'release_notes': app_info['release_notes']
                }

                if app_info['platform'] in 'win32':
                    params['is_enabled'] = app_info['is_enabled']
                    params['platform'] = app_info['platform_id']
                else:
                    app_info['darwindsasig'] = sign_update_sparkle(
                        source_file, os.environ.get('DSA_PRIVATE_PEM')).rstrip('\n')
                    params['dsa_signature'] = app_info['darwindsasig']
                    params['short_version'] = app_info['short_version']
            elif 'win64' in args.platform:
                target_version_id = get_omaha_version_id(channel, args.version, app_info['omahahost'],
                                                         app_info['headers'], logging)
                source_version_id = get_omaha_version_id(channel, args.previous, app_info['omahahost'],
                                                         app_info['headers'], logging)
                params = {
                    'target': target_version_id,
                    'source': source_version_id
                }

            response = post_with_file(app_info['version_post_url'], files, params, headers)

            if response.status_code != 201:
                logging.error("ERROR: Version not created! response.status_code : {}".format(response.status_code))
                logging.error("response.text : {}".format(response.text))

                if response.status_code == 400 and 'version must make a unique set' in response.text:
                    logging.error("ERROR: This version({}), channel({}), appguid({}) set has already been "
                                  "uploaded!".format(params['version'], app_info['channel'], params['app']))
                exit(1)

        if not app_info['previous'] and (app_info['platform'] in 'win32'):
            # When uploading windows builds, add actions to version just created
            rjson = response.json()

            if args.debug:
                logging.debug("response['id']: {}".format(rjson['id']))

            post_action(app_info['omahahost'], rjson['id'], 'install', headers, args)
            post_action(app_info['omahahost'], rjson['id'], 'update', headers, args)

    # if downloading from github, remove files after upload
    if args.github:
        remove_github_downloaded_files(file_list, logging)


if __name__ == '__main__':
    sys.exit(main())

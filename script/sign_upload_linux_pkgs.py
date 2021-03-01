#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import datetime
import logging
import os
import re
import requests
import subprocess
import sys
import time

from argparse import RawTextHelpFormatter
from lib.config import get_raw_version
from lib.helpers import *
from lib.github import GitHub


def main():

    args = parse_args()
    channel = args.channel
    repo_dir = args.repo_dir
    dist_dir = os.path.join(repo_dir, 'dist')
    gpg_full_key_id = args.gpg_full_key_id

    if args.skip_github and args.github_token:
        exit("Error: --skip_github and --github_token are mutually exclusive, only one allowed")

    if args.unmount is not False and channel in ['beta', 'dev', 'nightly']:
        unmount = args.unmount
    if channel in ['release']:
        if not args.gpg_passphrase:
            logging.error("Error: --gpg_passphrase required for channel {}".format(channel))
            exit(1)
        else:
            gpg_passphrase = args.gpg_passphrase
    s3_test_buckets = args.s3_test_buckets

    if os.environ.get('BRAVE_CORE_DIR'):
        brave_core_dir = os.environ.get('BRAVE_CORE_DIR')
    else:
        logging.error("Error: Required environment variable \'BRAVE_CORE_DIR\' not set! Exiting...")
        exit(1)

    if args.debug:
        logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)
        logging.debug('brave_version: {}'.format(get_raw_version()))
        logging.debug('channel: {}'.format(channel))
        logging.debug('repo_dir: {}'.format(repo_dir))
        logging.debug('dist_dir: {}'.format(dist_dir))
        logging.debug('gpg_full_key_id: {}'.format(gpg_full_key_id))
        logging.debug('gpg_passphrase: {}'.format("NOTAREALPASSWORD"))
        logging.debug('s3_test_buckets: {}'.format(s3_test_buckets))
        logging.debug('brave_core_dir: {}'.format(brave_core_dir))

    # verify we have the the GPG key we're expecting in the public keyring
    list_keys_cmd = "/usr/bin/gpg2 --list-keys --with-subkey-fingerprints | grep {}".format(
        gpg_full_key_id)
    logging.info("Verifying the GPG key \'{}\' is in our public keyring...".format(
        gpg_full_key_id))
    logging.debug("Running command: {}".format(list_keys_cmd))
    try:
        output = subprocess.check_output(list_keys_cmd, shell=True)
    except subprocess.CalledProcessError as cpe:
        logging.debug("Expected GPG ID not found in keyring!")
        logging.debug("Output from gpg2 --list-keys command: {}".format(cpe))
        if args.unmount is not False and channel in ['beta', 'dev', 'nightly']:
            logging.debug("Unmounting /home/ubuntu/.gnupg")
            list_keys_cmd = "/usr/bin/gpg2 --list-keys --with-subkey-fingerprints | grep {}".format(
                unmount)
            logging.debug("Running command: {}".format(list_keys_cmd))
            try:
                output = subprocess.check_output(list_keys_cmd, shell=True)
            except subprocess.CalledProcessError as cpe:
                logging.error("Error: {}".format(cpe))
                exit(1)
            try:
                unmount_cmd = "sudo umount /home/ubuntu/.gnupg"
                logging.debug("Running command: {}".format(unmount_cmd))
                output = subprocess.check_output(unmount_cmd, shell=True)
            except subprocess.CalledProcessError as cpe:
                logging.error("Error: {}".format(cpe))
                exit(1)

    saved_path = os.getcwd()
    try:
        os.chdir(dist_dir)
        logging.debug('Changed directory to \"{}\"'.format(dist_dir))
    except OSError as ose:
        message = (
            'Error: could not change directory to {}: {}'.format(dist_dir, ose))
        exit(message)

    if not args.skip_github:
        logging.info(
            "Downloading RPM/DEB packages to directory: {}".format(dist_dir))

        file_list = download_linux_pkgs_from_github(args, logging)

    try:
        os.chdir(repo_dir)
        logging.debug('Changed directory to \"{}\"'.format(repo_dir))
    except OSError as ose:
        message = (
            'Error: could not change directory to {}: {}'.format(repo_dir, ose))
        exit(message)

    # remove files older than 60 days from dist_dir
    delete_age = 60 * 86400
    # do not remove files that match this pattern
    global exclude_patterns
    exclude_patterns = ['.*keyring.*']

    logging.info("Performing removal of files older than 60 days in directory: {}".format(dist_dir))

    remove_files_older_x_days(dist_dir, delete_age, act=True)

    # If release channel, unlock GPG signing key which has a cache timeout of 30
    # minutes set in the gpg-agent.conf
    if channel in ['release']:
        gpgconf_cmd = ['gpgconf', '--kill', 'gpg-agent']
        logging.info("Running command: \"{}\"".format(gpgconf_cmd))
        try:
            subprocess.check_output(gpgconf_cmd, shell=True)
            logging.info("\"gpgconf --kill gpg-agent\" succeeded")
        except subprocess.CalledProcessError as cpe:
            logging.error("Error: {}".format(cpe))
            exit(1)
        cmd = ['gpg2', '--batch', '--pinentry-mode', 'loopback', '--passphrase',
               gpg_passphrase, '--sign']
        log_cmd = ['gpg2', '--batch', '--pinentry-mode', 'loopback', '--passphrase',
                   'NOTAREALPASSWORD', '--sign']
        logging.info("Running command: \"{}\"".format(log_cmd))
        try:
            p1 = subprocess.Popen(['echo'], stdout=subprocess.PIPE)
            p2 = subprocess.Popen(cmd, stdin=p1.stdout, stdout=subprocess.PIPE)
            p1.stdout.close()
            (stdoutdata, stderrdata) = p2.communicate()
            if stderrdata is not None:
                logging.error("subprocess.Popen.communicate() error: {}".format(stderrdata))
            logging.info("gpg2 unlock signing key successful!")
        except Exception as e:
            logging.error("Error running command: \"{}\"".format(log_cmd))
            exit(1)

    # Now upload to aptly and rpm repos

    for item in ['upload_to_aptly', 'upload_to_rpm_repo']:
        bucket = ''
        if re.match(r'.*rpm.*', item):
            bucket = 'brave-browser-rpm-staging-'
        else:
            bucket = 'brave-browser-apt-staging-'

        upload_script = os.path.join(brave_core_dir, 'script', item)

        TESTCHANNEL = 'test'

        if s3_test_buckets:
            upload_cmd = '{} {} {}'.format(upload_script, bucket + channel + '-' +
                                           TESTCHANNEL, gpg_full_key_id)
        else:
            upload_cmd = '{} {} {}'.format(
                upload_script, bucket + channel, gpg_full_key_id)
        logging.info("Running command: \"{}\"".format(upload_cmd))
        try:
            subprocess.check_output(upload_cmd, shell=True)
        except subprocess.CalledProcessError as cpe:
            logging.error("Error: {}".format(cpe))
            exit(1)

    # Not sure we need to change back to the original dir here,
    # keeping it for now.
    try:
        os.chdir(saved_path)
        logging.debug('Changed directory to \"{}\"'.format(saved_path))
    except OSError as ose:
        message = (
            'Error: could not change directory to {}: {}'.format(saved_path, ose))
        exit(message)


def locate_excludes(x):
    found = False
    for i in exclude_patterns:
        m = re.search(i, x)
        if m:
            found = True
    return not found


def remove_files_older_x_days(dir, age, act=False):
    items = get_files_older_x_days(dir, age)
    for i in items:
        logging.debug("Removing file: {}".format(i))
        if os.path.isfile(os.path.join(dir, i)):
            if act:
                logging.debug("Removing file: {}; "
                              " mtime: {}".format(i,
                                                  datetime.datetime.fromtimestamp(os.path.getmtime(
                                                                                            os.path.join(dir, i)))))
                try:
                    os.remove(os.path.join(dir, i))
                except Error as e:
                    logging.error("Cannot remove file: {}; Error: {}".format(os.path.join(dir, i), e))
            else:
                logging.debug("Would remove file(act=False): {}; "
                              " mtime: {}".format(i,
                                                  datetime.datetime.fromtimestamp(os.path.getmtime(
                                                                                            os.path.join(dir, i)))))


def get_files_older_x_days(dir, age):
    items = []
    now = time.time()
    files = os.listdir(dir)
    files = [f for f in filter(locate_excludes, files)]
    for f in files:
        path = os.path.join(dir, f)
        if os.path.isfile(path):
            if os.stat(path).st_mtime < now - age:
                items.append(f)
    return items


def download_linux_pkgs_from_github(args, logging):

    file_list = []

    # BRAVE_REPO defined in helpers.py
    repo = GitHub(args.github_token).repos(BRAVE_REPO)
    tag_name = args.tag
    release = {}
    releases = get_releases_by_tag(repo, tag_name, include_drafts=True)
    if releases:
        if len(releases) > 1:
            exit("Error: More than 1 release exists with the tag: \'{}\'".format(tag_name))
        release = releases[0]
    if release['assets'] is None:
        logging.error(
            'Error: Could not find GitHub release with tag {}. Exiting...'.format(tag_name))
        exit(1)
    else:
        logging.info(
            "Searching for RPM/DEB packages in GitHub release: {}".format(release['url']))
    for asset in release['assets']:
        if re.match(r'.*\.rpm$', asset['name']) \
                or re.match(r'.*\.deb$', asset['name']):
            filename = asset['name']
            asset_id = asset['id']
            asset_url = asset['url']
            if args.debug:
                logging.debug("GitHub asset_url: {}".format(
                    asset_url + '/' + filename))

            # Check if the file exists on disk first, and remove it if found.
            # This prevents a situation where the expect script fails because
            # the rpm has already been signed.
            remove_existing_file(filename, logging)

            # Instantiate new requests session, versus reusing the repo session above.
            # Headers was likely being reused in that session, and not allowing us
            # to set the Accept header to the below.
            perform_github_download(
                asset_url, args, logging, filename, file_list)

    if len(file_list) < 2:
        logging.error(
            "Cannot get both RPM and DEB files from GitHub! "
            "Removing partially downloaded files from directory: {}".format(dist_dir))
        remove_github_downloaded_files(file_list, logging)
        exit(1)
    return file_list


def perform_github_download(asset_url, args, logging, filename, file_list):
    # Instantiate new requests session, versus reusing the repo session above.
    # Headers was likely being reused in that session, and not allowing us
    # to set the Accept header to the below.
    headers = {'Accept': 'application/octet-stream',
               'Authorization': 'token ' + args.github_token}
    if args.debug:
        # disable urllib3 logging for this session to avoid showing
        # access_token in logs
        logging.getLogger("urllib3").setLevel(logging.WARNING)
    logging.info("Downloading GitHub release asset: {}".format(
        asset_url + '/' + filename))
    try:
        r = requests.get(asset_url, headers=headers, stream=True)
    except requests.exceptions.ConnectionError as e:
        logging.error(
            "Error: Received requests.exceptions.ConnectionError, Exiting...")
        exit(1)
    except Exception as e:
        logging.error(
            "Error: Received exception {},  Exiting...".format(type(e)))
        exit(1)
    if args.debug:
        logging.getLogger("urllib3").setLevel(logging.DEBUG)
    with open(filename, 'wb') as f:
        for chunk in r.iter_content(chunk_size=1024):
            if chunk:
                f.write(chunk)

    logging.debug(
        "Requests Response status_code: {}".format(r.status_code))
    if r.status_code == 200:
        logging.info(
            "Download successful: Response Status Code {}".format(r.status_code))
        file_list.append('./' + filename)
    else:
        logging.debug(
            "Requests Response status_code != 200: {}".format(r.status_code))


def remove_existing_file(filename, logging):
    if os.path.isfile(os.path.join(os.getcwd(), filename)):
        logging.info("File \'{}\' already exists, removing...".format(filename))
        try:
            os.remove(filename)
        except Exception as e:
            logging.error(
                "Error: could not remove file {}: {}".format(filename, e))


def parse_args():
    desc = "Download Linux packages from GitHub, sign them, then upload to apt/rpm repositories"

    parser = argparse.ArgumentParser(description=desc, formatter_class=RawTextHelpFormatter)
    parser.add_argument('-c', '--channel', help='The Brave channel, i.e. \'nightly\', \'dev\', \'beta\', \'release\'',
                        required=True)
    parser.add_argument('-d', '--debug', action='store_true',
                        help='Print debug output')
    parser.add_argument('-g', '--github_token',
                        help='GitHub token to use for downloading releases(cannot be combined with --skip_github)',
                        action='store_true')
    parser.add_argument('-k', '--gpg_full_key_id', help='GPG full key id to use for signing '
                        'packages', required=True)
    parser.add_argument('-n', '--skip_github', help='Skip downloading from GitHub, assume packages are '
                        'in the repo_dir already(cannot be combined with --github_token)', action='store_true')
    parser.add_argument(
        '-t', '--tag', help='The branch (actually tag) to download packages from GitHub. (i.e. v1.5.18)',
        required=True)
    parser.add_argument('-p', '--gpg_passphrase',
                        help='GPG passphrase to unlock signing keychain')
    parser.add_argument('-r', '--repo_dir', help='Directory on upload server to download RPM/DEB files into',
                        required=True)
    parser.add_argument('-s', '--s3_test_buckets', help='Upload to test S3 buckets (same names but with'
                        ' \'-test\' postfix) for QA testing', action='store_true')
    parser.add_argument('-u', '--unmount', help='Unmount the ~/.gnupg filesystem if this key is found in'
                        ' the GPG Keyring', required=False)
    return parser.parse_args()


if __name__ == '__main__':
    sys.exit(main())

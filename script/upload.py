#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import errno
import logging
import os
import requests
import re
import shutil
import subprocess
import sys
import tempfile
from lib.github import GitHub
from lib.helpers import *
from lib.config import (PLATFORM, get_env_var, product_name,
                        project_name, SOURCE_ROOT, dist_dir, output_dir,
                        get_brave_version, get_platform_key, get_raw_version)
from lib.util import execute, parse_version, scoped_cwd, s3put


if os.environ.get('DEBUG_HTTP_HEADERS') == 'true':
    try:
        from http.client import HTTPConnection  # python3
    except ImportError:
        from httplib import HTTPConnection  # python2


def get_zip_name(name, version, target_arch, suffix=''):
    arch = 'ia32' if (target_arch == 'x86') else target_arch
    if arch == 'arm':
        arch += 'v7l'
    zip_name = '{0}-{1}-{2}-{3}'.format(name, version, get_platform_key(),
                                        arch)
    if suffix:
        zip_name += '-' + suffix
    return zip_name + '.zip'


def main():
    args = parse_args()
    print('[INFO] Running upload...')

    # Enable urllib3 debugging output
    if os.environ.get('DEBUG_HTTP_HEADERS') == 'true':
        logging.basicConfig(level=logging.DEBUG)
        logging.getLogger("urllib3").setLevel(logging.DEBUG)
        logging.debug("DEBUG_HTTP_HEADERS env var is enabled, logging HTTP headers")
        debug_requests_on()

    # BRAVE_REPO is defined in lib/helpers.py for now
    repo = GitHub(get_env_var('GITHUB_TOKEN')).repos(BRAVE_REPO)

    tag = get_brave_version()
    release = get_release(repo, tag, allow_published_release_updates=False)

    if not release:
        print("[INFO] No existing release found, creating new "
              "release for this upload")
        release = create_release_draft(repo, tag)

    d_dir = dist_dir(args.target_os, args.target_arch)
    o_dir = output_dir(args.target_os, args.target_arch)

    DIST_NAME = get_zip_name(project_name(), get_brave_version(), args.target_arch)
    SYMBOLS_NAME = get_zip_name(project_name(), get_brave_version(), args.target_arch, 'symbols')
    DSYM_NAME = get_zip_name(project_name(), get_brave_version(), args.target_arch, 'dsym')
    PDB_NAME = get_zip_name(project_name(), get_brave_version(), args.target_arch, 'pdb')

    print('[INFO] Uploading release {}'.format(release['tag_name']))
    # Upload Brave with GitHub Releases API.
    if args.target_os != 'android':
        upload_brave(repo, release, os.path.join(d_dir, DIST_NAME), force=args.force)
        upload_brave(repo, release, os.path.join(d_dir, SYMBOLS_NAME), force=args.force)
    else:
        o_dir = o_dir + '/apks'
    # if PLATFORM == 'darwin':
    #     upload_brave(repo, release, os.path.join(d_dir, DSYM_NAME))
    # elif PLATFORM == 'win32':
    #     upload_brave(repo, release, os.path.join(d_dir, PDB_NAME))

    pkgs = get_brave_packages(o_dir, release_channel(), get_raw_version(),
                              args.target_os, args.target_arch, args.target_apk_base)

    for pkg in pkgs:
        upload_brave(repo, release, os.path.join(o_dir, pkg), force=args.force)

    # mksnapshot = get_zip_name('mksnapshot', get_brave_version(), args.target_arch)
    # upload_brave(repo, release, os.path.join(d_dir, mksnapshot))

    # if PLATFORM == 'win32' and not tag_exists:
    #     # Upload PDBs to Windows symbol server.
    #     run_python_script('upload-windows-pdb.py')

    if os.environ.get('DEBUG_HTTP_HEADERS') == 'true':
        debug_requests_off()
    print('[INFO] Finished upload')


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


def get_brave_packages(dir, channel, version, target_os, target_arch='x64', target_apk_base=''):
    pkgs = []

    def filecopy(file_path, file_desired):
        file_desired_path = os.path.join(dir, file_desired)
        if os.path.isfile(file_path):
            print('[INFO] Copying file ' + file_path + ' to ' + file_desired_path)
            shutil.copy(file_path, file_desired_path)
        return file_desired_path

    channel_capitalized = '' if (channel == 'release') else channel.capitalize()
    for file in os.listdir(dir):
        if os.path.isfile(os.path.join(dir, file)):
            file_path = os.path.join(dir, file)
            if PLATFORM == 'darwin':
                channel_capitalized_dashed = '' if (channel == 'release') else ('-' + channel_capitalized)
                channel_capitalized_spaced = '' if (channel == 'release') else (' ' + channel_capitalized)
                file_dmg = 'Brave-Browser' + channel_capitalized_dashed + '.dmg'
                file_pkg = 'Brave-Browser' + channel_capitalized_dashed + '.pkg'
                if re.match(r'Brave Browser' + channel_capitalized_spaced + r'.*\.dmg$', file):
                    filecopy(file_path, file_dmg)
                    pkgs.append(file_dmg)
                elif file == file_dmg:
                    pkgs.append(file_dmg)
                elif re.match(r'Brave Browser' + channel_capitalized_spaced + r'.*\.pkg$', file):
                    filecopy(file_path, file_pkg)
                    pkgs.append(file_pkg)
                elif file == file_pkg:
                    pkgs.append(file_pkg)
            elif PLATFORM == 'linux':
                channel_dashed = '' if (channel == 'release') else ('-' + channel)
                if re.match(r'brave-browser' + channel_dashed + '_' + version + r'.*\.deb$', file):
                    pkgs.append(file)
                elif re.match(r'brave-browser' + channel_dashed + '-' + version + r'.*\.rpm$', file):
                    pkgs.append(file)
                elif target_os == 'android':
                    if target_arch == 'arm':
                        if not target_apk_base:
                            if re.match(r'Brave.*arm.apk$', file):
                                pkgs.append(file)
                        else:
                            if target_apk_base == 'classic':
                                if file == 'Bravearm.apk':
                                    pkgs.append(file)
                            elif target_apk_base == 'modern':
                                if file == 'BraveModernarm.apk':
                                    pkgs.append(file)
                            elif target_apk_base == 'mono':
                                if file == 'BraveMonoarm.apk':
                                    pkgs.append(file)
                    elif target_arch == 'arm64':
                        if file == 'BraveMonoarm64.apk':
                            pkgs.append(file)
                    elif target_arch == 'x86':
                        if not target_apk_base:
                            if re.match(r'Brave.*x86.apk$', file):
                                pkgs.append(file)
                        else:
                            if target_apk_base == 'classic':
                                if file == 'Bravex86.apk':
                                    pkgs.append(file)
                            elif target_apk_base == 'modern':
                                if file == 'BraveModernx86.apk':
                                    pkgs.append(file)
                            elif target_apk_base == 'mono':
                                if file == 'BraveMonox86.apk':
                                    pkgs.append(file)
                    elif target_arch == 'x64':
                        if file == 'BraveMonox64.apk':
                            pkgs.append(file)
                    else:
                        if re.match(r'Brave.*.apk$', file):
                            pkgs.append(file)
            elif PLATFORM == 'win32':
                target_arch = 'ia32' if (target_arch == 'x86') else target_arch
                arch = '32' if (target_arch == 'ia32') else ''
                channel_arch_extension = channel_capitalized + 'Setup' + arch + '.exe'
                file_stub = 'BraveBrowser' + channel_arch_extension
                file_stub_untagged = 'BraveBrowserUntagged' + channel_arch_extension
                file_stn = 'BraveBrowserStandalone' + channel_arch_extension
                file_stn_silent = 'BraveBrowserStandaloneSilent' + channel_arch_extension
                file_stn_untagged = 'BraveBrowserStandaloneUntagged' + channel_arch_extension
                file_installer = 'brave_installer-' + target_arch + '.exe'

                if re.match(r'BraveBrowser' + channel_capitalized + r'Setup' + arch + r'_.*\.exe', file):
                    filecopy(file_path, file_stub)
                    pkgs.append(file_stub)
                elif re.match(r'BraveBrowserUntagged' + channel_capitalized + r'Setup' + arch + r'_.*\.exe', file):
                    filecopy(file_path, file_stub_untagged)
                    pkgs.append(file_stub_untagged)
                elif re.match(r'BraveBrowserStandalone' + channel_capitalized + r'Setup' + arch + r'_.*\.exe', file):
                    filecopy(file_path, file_stn)
                    pkgs.append(file_stn)
                elif re.match(r'BraveBrowserStandaloneSilent' + channel_capitalized + r'Setup' + arch +
                              r'_.*\.exe', file):
                    filecopy(file_path, file_stn_silent)
                    pkgs.append(file_stn_silent)
                elif re.match(r'BraveBrowserStandaloneUntagged' + channel_capitalized + r'Setup' + arch +
                              r'_.*\.exe', file):
                    filecopy(file_path, file_stn_untagged)
                    pkgs.append(file_stn_untagged)
                elif re.match(r'brave_installer.exe', file):
                    filecopy(file_path, file_installer)
                    pkgs.append(file_installer)

    return sorted(list(set(pkgs)))


def parse_args():
    parser = argparse.ArgumentParser(description='upload distribution file')
    parser.add_argument('--force', action='store_true',
                        help='Overwrite files in '
                        'destination draft on upload.')
    parser.add_argument('-v', '--version',
                        help='Specify the version',
                        default=get_brave_version())
    parser.add_argument('--target_os',
                        help='Specify the target OS',
                        default='')
    parser.add_argument('--target_arch',
                        help='Specify the target arch',
                        default='')
    parser.add_argument('--target_apk_base',
                        help='Specify the target APK base',
                        default='')
    return parser.parse_args()


def run_python_script(script, *args):
    script_path = os.path.join(SOURCE_ROOT, 'script', script)
    return execute([sys.executable, script_path] + list(args))


def get_text_with_editor(name):
    editor = os.environ.get('EDITOR', 'nano')
    initial_message = (
        '\n# Please enter the body of your release note for %s.' % name)

    t = tempfile.NamedTemporaryFile(suffix='.tmp', delete=False)
    t.write(initial_message)
    t.close()
    subprocess.call([editor, t.name])

    text = ''
    for line in open(t.name, 'r'):
        if len(line) == 0 or line[0] != '#':
            text += line

    os.unlink(t.name)
    return text


def create_release_draft(repo, tag):
    name = '{0} {1}'.format(release_name(), tag)
    channel = release_channel()
    channel_capitalized_dashed = '' if (channel == 'release') else ('-' + channel.capitalize())

    # TODO: Parse release notes from CHANGELOG.md

    nightly_winstallers = (
        '`BraveBrowserNightlySetup.exe` and `BraveBrowserNightlySetup32.exe`')
    dev_winstallers = (
        '`BraveBrowserDevSetup.exe` and `BraveBrowserDevSetup32.exe`')
    beta_winstallers = (
        '`BraveBrowserBetaSetup.exe` and `BraveBrowserBetaSetup32.exe`')
    release_winstallers = (
        '`BraveBrowserSetup.exe` and `BraveBrowserSetup32.exe`')

    nightly_dev_beta_warning = '''*This is not the released version of Brave.
**Be careful** - things are unstable and might even be broken.*

These builds are an unpolished and unfinished early preview for the new \
version of Brave on the desktop. These builds show our work in progress and \
they aren't for the faint-of-heart. Features may be missing or broken in new \
and exciting ways; familiar functionality may have unfamiliar side-effects. \
These builds showcase the newest advances that we're bringing to your browser, \
but this is still a prototype, not a reliable daily driver. Try it out only if \
you're looking for a little extra spice and adventure in your browsing.'''

    if channel == 'nightly':
        winstallers = nightly_winstallers
        warning = nightly_dev_beta_warning
    elif channel == 'dev':
        winstallers = dev_winstallers
        warning = nightly_dev_beta_warning
    elif channel == 'beta':
        winstallers = beta_winstallers
        warning = nightly_dev_beta_warning
    else:
        winstallers = release_winstallers
        warning = ""

    body = '''{warning}

# Mac installation
Install Brave-Browser{channel_capitalized_dashed}.dmg on your system.

# Linux install instructions
https://brave.com/linux

# Windows
{win} will fetch and install the latest available version from our \
update server.'''.format(warning=warning, channel_capitalized_dashed=channel_capitalized_dashed, win=winstallers)

    data = dict(tag_name=tag, name=name, body=body, draft=True)

    release = retry_func(lambda run: repo.releases.post(data=data),
                         catch=requests.exceptions.ConnectionError, retries=3)
    return release


def upload_brave(github, release, file_path, filename=None, force=False):
    # Delete the original file before uploading.
    if filename is None:
        filename = os.path.basename(file_path)

    try:
        for asset in release['assets']:
            if asset['name'] == filename:
                print('[INFO] Asset "' + filename + '" exists; deleting...')
                github.repos(BRAVE_REPO).releases.assets(asset['id']).delete()
                print('[INFO] Asset "' + filename + '" deleted')
    except Exception:
        pass

    # Upload the file.
    print('[INFO] Uploading: ' + filename)
    with open(file_path, 'rb') as f:
        if force:
            print('[INFO] force deleting "' + filename + '" before upload...')
            delete_file(github, release, filename)
            print('[INFO] force deleted "' + filename + '".')

        retry_func(
            lambda ran: upload_io_to_github(github, release, filename, f, 'application/zip'),
            catch_func=lambda ran: delete_file(github, release, filename),
            catch=requests.exceptions.ConnectionError, retries=3
        )

    # Upload ARM assets without the v7l suffix for backwards compatibility
    # TODO Remove for 2.0
    if 'armv7l' in filename:
        arm_filename = filename.replace('armv7l', 'arm')
        arm_file_path = os.path.join(os.path.dirname(file_path), arm_filename)
        shutil.copy2(file_path, arm_file_path)
        upload_brave(github, release, arm_file_path)


def upload_io_to_github(github, release, name, io, content_type):
    io.seek(0)
    github.releases(release['id']).assets.post(
        params={'name': name},
        headers={'Content-Type': content_type},
        data=io,
        verify=False
    )


def auth_token():
    token = get_env_var('GITHUB_TOKEN')
    message = ('Error: Please set the $BRAVE_GITHUB_TOKEN '
               'environment variable, which is your personal token')
    assert token, message
    return token


def delete_file(github, release, name, retries=3):
    release = retry_func(
        lambda run: github.releases(release['id']).get(),
        catch=requests.exceptions.ConnectionError, retries=3
    )
    for asset in release['assets']:
        if asset['name'] == name:
            print("[INFO] Deleting file name '{}' with asset id {}".format(name, asset['id']))
            retry_func(lambda run: github.releases.assets(asset['id']).delete(),
                       catch=requests.exceptions.ConnectionError, retries=3)


if __name__ == '__main__':
    import sys
    sys.exit(main())

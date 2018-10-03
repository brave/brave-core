#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import errno
import hashlib
import os
import requests
import re
import shutil
import subprocess
import sys
import tempfile

from io import StringIO
from lib.config import PLATFORM, DIST_URL, get_target_arch, get_chromedriver_version, \
                       get_env_var, s3_config, get_zip_name, product_name, project_name, \
                       SOURCE_ROOT, dist_dir, output_dir, get_brave_version, get_raw_version
from lib.util import execute, parse_version, scoped_cwd, s3put
from lib.helpers import *

from lib.github import GitHub

DIST_NAME = get_zip_name(project_name(), get_brave_version())
SYMBOLS_NAME = get_zip_name(project_name(), get_brave_version(), 'symbols')
DSYM_NAME = get_zip_name(project_name(), get_brave_version(), 'dsym')
PDB_NAME = get_zip_name(project_name(), get_brave_version(), 'pdb')


def main():
  args = parse_args()
  print('[INFO] Running upload...')

  # Repo is defined in lib/helpers.py for now
  repo = GitHub(get_env_var('GITHUB_TOKEN')).repos(BRAVE_REPO)

  tag = get_brave_version()
  release = get_draft(repo, tag)

  if not release:
    print("[INFO] No existing release found, creating new release for this upload")
    release = create_release_draft(repo, tag)

  print('[INFO] Uploading release {}'.format(release['tag_name']))
  # Upload Brave with GitHub Releases API.
  upload_brave(repo, release, os.path.join(dist_dir(), DIST_NAME), force=args.force)
  upload_brave(repo, release, os.path.join(dist_dir(), SYMBOLS_NAME), force=args.force)
  # if PLATFORM == 'darwin':
  #   upload_brave(repo, release, os.path.join(dist_dir(), DSYM_NAME))
  # elif PLATFORM == 'win32':
  #   upload_brave(repo, release, os.path.join(dist_dir(), PDB_NAME))

  # Upload chromedriver and mksnapshot.
  chromedriver = get_zip_name('chromedriver', get_chromedriver_version())
  upload_brave(repo, release, os.path.join(dist_dir(), chromedriver), force=args.force)

  pkgs = yield_brave_packages(output_dir(), release_channel(), get_raw_version())

  if PLATFORM == 'darwin':
    for pkg in pkgs:
      upload_brave(repo, release, os.path.join(output_dir(), pkg), force=args.force)
  elif PLATFORM == 'win32':
    if get_target_arch() == 'x64':
      upload_brave(repo, release, os.path.join(output_dir(),
          'brave_installer.exe'), 'brave_installer-x64.exe', force=args.force)
    else:
      upload_brave(repo, release, os.path.join(output_dir(),
          'brave_installer.exe'), 'brave_installer-ia32.exe', force=args.force)
  else:
    if get_target_arch() == 'x64':
      for pkg in pkgs:
        upload_brave(repo, release, os.path.join(output_dir(), pkg), force=args.force)
    else:
      upload_brave(repo, release, os.path.join(output_dir(), 'brave-i386.rpm'), force=args.force)
      upload_brave(repo, release, os.path.join(output_dir(), 'brave-i386.deb'), force=args.force)

  # mksnapshot = get_zip_name('mksnapshot', get_brave_version())
  # upload_brave(repo, release, os.path.join(dist_dir(), mksnapshot))

  # if PLATFORM == 'win32' and not tag_exists:
  #   # Upload PDBs to Windows symbol server.
  #   run_python_script('upload-windows-pdb.py')

  versions = parse_version(args.version)
  version = '.'.join(versions[:3])
  print('[INFO] Finished upload')


def yield_brave_packages(dir, channel, version):
  # NOTE: mbacchi - before official release this must handle stable release channel which is ""
  for _, _, files in os.walk(dir):
    for file in files:
      if PLATFORM == 'darwin':
        sought_file = 'Brave-Browser-' + channel.capitalize() + '.dmg'
        if re.match(r'Brave Browser ' + channel.capitalize() + r'.*\.dmg$', file):
          os.rename(file, sought_file)
          yield sought_file
      elif PLATFORM == 'linux':
        if re.match(r'brave-browser-' + channel + '_' + version + r'.*\.deb$', file) \
          or re.match(r'brave-browser-' + channel + '-' + version + r'.*\.rpm$', file):
          yield file

def get_draft(repo, tag):
  release = None
  releases = get_releases_by_tag(repo, tag, include_drafts=True)
  if releases:
    print("[INFO] Found existing release draft, merging this upload with it")
    if len(releases) > 1:
      raise UserWarning("[INFO] More then one draft with the tag '{}' found, not sure which one to merge with.".format(tag))
    release = releases[0]
    if release['draft'] == False:
      raise UserWarning("[INFO] Release with tag '{}' is already published, aborting.".format(tag))

  return release


def parse_args():
  parser = argparse.ArgumentParser(description='upload distribution file')
  parser.add_argument('--force', action='store_true', 
                      help='Overwrite files in destination draft on upload.')
  parser.add_argument('-v', '--version', help='Specify the version',
                      default=get_brave_version())
  parser.add_argument('-d', '--dist-url',
                      help='The base dist url for download',
                      default=DIST_URL)
  return parser.parse_args()


def run_python_script(script, *args):
  script_path = os.path.join(SOURCE_ROOT, 'script', script)
  return execute([sys.executable, script_path] + list(args))


def dist_newer_than_head():
  with scoped_cwd(SOURCE_ROOT):
    try:
      head_time = subprocess.check_output(['git', 'log', '--pretty=format:%at',
                                           '-n', '1']).strip()
      dist_time = os.path.getmtime(os.path.join(dist_dir(), DIST_NAME))
    except OSError as e:
      if e.errno != errno.ENOENT:
        raise
      return False

  return dist_time > int(head_time)


def get_text_with_editor(name):
  editor = os.environ.get('EDITOR', 'nano')
  initial_message = '\n# Please enter the body of your release note for %s.' \
                    % name

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
  # TODO: Parse release notes from CHANGELOG.md
  body = '''*This is not the released version of Brave. **Be careful** - things are unstable and might even be broken.*

These builds are an unpolished and unfinished early preview for the new version of Brave on the desktop. These builds show our work in progress and they aren't for the faint-of-heart. Features may be missing or broken in new and exciting ways; familiar functionality may have unfamiliar side-effects. These builds showcase the newest advances that we're bringing to your browser, but this is still a prototype, not a reliable daily driver. Try it out only if you're looking for a little extra spice and adventure in your browsing.

### Linux install instructions
http://brave-browser.readthedocs.io/en/latest/installing-brave.html#linux

### Windows
`BraveBrowserDevSetup.exe` will fetch and install the latest available version.

Until we have brave_installer.exe working without the need for adding cmd line
args it is unlisted in favor of the stub installer above to avoid confusion.'''

  data = dict(tag_name=tag, name=name, body=body, draft=True)

  release = retry_func(
    lambda run: repo.releases.post(data=data),
    catch=requests.exceptions.ConnectionError, retries=3
  )
  return release


def upload_brave(github, release, file_path, filename=None, force=False):
  # Delete the original file before uploading.
  if filename == None:
    filename = os.path.basename(file_path)

  print('[INFO] Uploading: ' + filename)
  try:
    for asset in release['assets']:
      if asset['name'] == filename:
        github.repos(BRAVE_REPO).releases.assets(asset['id']).delete()
  except Exception:
    pass

  # Upload the file.
  with open(file_path, 'rb') as f:
    if force:
      delete_file(github, release, filename)

    retry_func(
      lambda ran: upload_io_to_github(github, release, filename, f, 'application/zip'),
      catch_func=lambda ran: delete_file(github, release, filename),
      catch=requests.exceptions.ConnectionError, retries=3
    )

  # Upload the checksum file.
  upload_sha256_checksum(release['tag_name'], file_path)

  # Upload ARM assets without the v7l suffix for backwards compatibility
  # TODO Remove for 2.0
  if 'armv7l' in filename:
    arm_filename = filename.replace('armv7l', 'arm')
    arm_file_path = os.path.join(os.path.dirname(file_path), arm_filename)
    shutil.copy2(file_path, arm_file_path)
    upload_brave(github, release, arm_file_path)


def upload_io_to_github(github, release, name, io, content_type, retries=3):
  io.seek(0)
  github.releases(release['id']).assets.post(
  params={'name': name},
  headers={'Content-Type': content_type},
  data=io, verify=False
  )


def upload_sha256_checksum(version, file_path):
  bucket, access_key, secret_key = s3_config()
  checksum_path = '{}.sha256sum'.format(file_path)
  sha256 = hashlib.sha256()
  with open(file_path, 'rb') as f:
    sha256.update(f.read())

  filename = os.path.basename(file_path)
  with open(checksum_path, 'w') as checksum:
    checksum.write('{} *{}'.format(sha256.hexdigest(), filename))
  s3put(bucket, access_key, secret_key, os.path.dirname(checksum_path),
        'releases/tmp/{0}'.format(version), [checksum_path])


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
      retry_func(
        lambda run: github.releases.assets(asset['id']).delete(),
        catch=requests.exceptions.ConnectionError, retries=3
      )


if __name__ == '__main__':
  import sys
  sys.exit(main())


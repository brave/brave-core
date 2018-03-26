#!/usr/bin/env python

import argparse
import errno
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

from io import StringIO
from lib.config import PLATFORM, DIST_URL, get_target_arch, get_chromedriver_version, \
                       get_env_var, s3_config, get_zip_name, product_name, project_name, \
                       SOURCE_ROOT, dist_dir, output_dir, get_brave_version
from lib.util import execute, parse_version, scoped_cwd, s3put

from lib.github import GitHub


BRAVE_REPO = "brave/brave-browser-builds"

DIST_NAME = get_zip_name(project_name(), get_brave_version())
SYMBOLS_NAME = get_zip_name(project_name(), get_brave_version(), 'symbols')
DSYM_NAME = get_zip_name(project_name(), get_brave_version(), 'dsym')
PDB_NAME = get_zip_name(project_name(), get_brave_version(), 'pdb')


def main():
  args = parse_args()

  if not args.publish_release:
    build_version = get_brave_version()
    if not get_brave_version().startswith(build_version):
      error = 'Tag name ({0}) should match build version ({1})\n'.format(
          get_brave_version(), build_version)
      sys.stderr.write(error)
      sys.stderr.flush()
      return 1

  github = GitHub(auth_token())
  releases = github.repos(BRAVE_REPO).releases.get()
  tag_exists = False
  for release in releases:
    if not release['draft'] and release['tag_name'] == args.version:
      tag_exists = True
      break

  release = create_or_get_release_draft(github, releases, args.version,
                                        tag_exists)

  if args.publish_release:
    # Create and upload the Brave SHASUMS*.txt
    release_brave_checksums(github, release)

    # Press the publish button.
    # publish_release(github, release['id'])

    # Do not upload other files when passed "-p".
    return

  # Upload Brave with GitHub Releases API.
  upload_brave(github, release, os.path.join(dist_dir(), DIST_NAME))
  upload_brave(github, release, os.path.join(dist_dir(), SYMBOLS_NAME))
  # if PLATFORM == 'darwin':
  #   upload_brave(github, release, os.path.join(dist_dir(), DSYM_NAME))
  # elif PLATFORM == 'win32':
  #   upload_brave(github, release, os.path.join(dist_dir(), PDB_NAME))

  # Upload chromedriver and mksnapshot.
  chromedriver = get_zip_name('chromedriver', get_chromedriver_version())
  upload_brave(github, release, os.path.join(dist_dir(), chromedriver))

  if PLATFORM == 'darwin':
    upload_brave(github, release, os.path.join(output_dir(), 'Brave.dmg'))
  elif PLATFORM == 'win32':
    if get_target_arch() == 'x64':
      upload_brave(github, release, os.path.join(output_dir(),
          'brave_installer.exe'), 'brave_installer-x64.exe')
    else:
      upload_brave(github, release, os.path.join(output_dir(),
          'brave_installer.exe'), 'brave_installer-ia32.exe')
  # TODO: Enable after linux packaging lands
  #else:
    #if get_target_arch() == 'x64':
      #upload_brave(github, release, os.path.join(output_dir(), 'brave-x86_64.rpm'))
      #upload_brave(github, release, os.path.join(output_dir(), 'brave-amd64.deb'))
    #else:
      #upload_brave(github, release, os.path.join(output_dir(), 'brave-i386.rpm'))
      #upload_brave(github, release, os.path.join(output_dir(), 'brave-i386.deb'))

  # mksnapshot = get_zip_name('mksnapshot', get_brave_version())
  # upload_brave(github, release, os.path.join(dist_dir(), mksnapshot))

  # if PLATFORM == 'win32' and not tag_exists:
  #   # Upload PDBs to Windows symbol server.
  #   run_python_script('upload-windows-pdb.py')

  versions = parse_version(args.version)
  version = '.'.join(versions[:3])


def parse_args():
  parser = argparse.ArgumentParser(description='upload distribution file')
  parser.add_argument('-v', '--version', help='Specify the version',
                      default=get_brave_version())
  parser.add_argument('-p', '--publish-release',
                      help='Publish the release',
                      action='store_true')
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

def create_or_get_release_draft(github, releases, tag, tag_exists):
  # Search for existing draft.
  for release in releases:
    if release['draft']:
      return release

  if tag_exists:
    tag = 'do-not-publish-me'

  return create_release_draft(github, tag)


def create_release_draft(github, tag):
  name = '{0} {1}'.format(project_name(), tag)
  body = '(placeholder)'
  data = dict(tag_name=tag, name=name, body=body, draft=True)
  r = github.repos(BRAVE_REPO).releases.post(data=data)
  return r


def release_brave_checksums(github, release):
  checksums = run_python_script('merge-brave-checksums.py',
                                '-v', get_brave_version())
  upload_io_to_github(github, release, 'SHASUMS256.txt',
                      StringIO(checksums.decode('utf-8')), 'text/plain')


def upload_brave(github, release, file_path, filename=None):
  # Delete the original file before uploading.
  if filename == None:
    filename = os.path.basename(file_path)

  try:
    for asset in release['assets']:
      if asset['name'] == filename:
        github.repos(BRAVE_REPO).releases.assets(asset['id']).delete()
  except Exception:
    pass

  # Upload the file.
  with open(file_path, 'rb') as f:
    upload_io_to_github(github, release, filename, f, 'application/zip')

  # Upload the checksum file.
  upload_sha256_checksum(release['tag_name'], file_path)

  # Upload ARM assets without the v7l suffix for backwards compatibility
  # TODO Remove for 2.0
  if 'armv7l' in filename:
    arm_filename = filename.replace('armv7l', 'arm')
    arm_file_path = os.path.join(os.path.dirname(file_path), arm_filename)
    shutil.copy2(file_path, arm_file_path)
    upload_brave(github, release, arm_file_path)


def upload_io_to_github(github, release, name, io, content_type):
  params = {'name': name}
  headers = {'Content-Type': content_type}
  github.repos(BRAVE_REPO).releases(release['id']).assets.post(
      params=params, headers=headers, data=io, verify=False)


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


def publish_release(github, release_id):
  data = dict(draft=False)
  github.repos(BRAVE_REPO).releases(release_id).patch(data=data)


def auth_token():
  token = get_env_var('GITHUB_TOKEN')
  message = ('Error: Please set the $BRAVE_GITHUB_TOKEN '
             'environment variable, which is your personal token')
  assert token, message
  return token


if __name__ == '__main__':
  import sys
  sys.exit(main())


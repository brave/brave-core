# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
import base64
import cryptography
import logging
import os

from lib.config import get_brave_version, get_chrome_version, get_raw_version
from lib.connect import post, get
from lib.helpers import release_channel
from lib.util import omaha_channel

# Event IDs never change
event_id = {
    'preinstall': 0,
    'install': 1,
    'postinstall': 2,
    'update': 3
}

# Platform IDs never change
platform_id = {
    'win': 1,
    'mac': 2
}


def get_channel_id(channel, host, headers, logging):
    channel_json = get_channel_ids_from_omaha_server(host, headers, logging)
    id = next(item['id'] for item in channel_json if item['name'] == channel)
    return id


def get_channel_ids_from_omaha_server(host, headers, logging):
    '''
    Channel IDs can change between Omaha servers and over time
    may even change on the same Omaha server. Get all IDs from
    the server versus hardcoding in an enum like we do above
    with Event IDs or Platform IDs.
    '''
    url = 'https://' + host + '/api/channel'
    response = get(url, headers)
    if response.status_code != 200:
        logging.error('ERROR: Cannot GET /api/channel from Omaha host {}! '
                      'response.status_code: {}'.format(host, response.status_code))
        logging.error(response.raise_for_status())
        exit(1)
    return response.json()


def get_omaha_version_id(channel, version, host, headers, logging):
    channel_id = get_channel_id(channel, host, headers, logging)
    url = 'https://' + host + '/api/omaha/version?version=' + version
    response = get(url, headers)
    if response.status_code != 200:
        logging.error('ERROR: Cannot GET /api/omaha/version from Omaha host {}! '
                      'response.status_code: {}'.format(host, response.status_code))
        logging.error(response.raise_for_status())
        exit(1)
    id = next(item['id'] for item in response.json() if (item['channel'] == channel_id and item['version'] == version))
    return id


def get_event_id(event):
    return event_id[event]


def get_platform_id(platform):
    if 'win' in platform:
        platform = 'win'
    if 'darwin' in platform:
        platform = 'mac'
    return platform_id[platform]


def get_base64_authorization(omahaid, omahapw):
    '''
    Returns a base64 encoded string created from the Omaha ID and PW
    '''

    concatstr = omahaid + ':' + omahapw
    return base64.b64encode(concatstr.encode())


# TODO: add functions to create apps
def get_appguid(channel, platform):
    if channel in ['dev'] or (platform in ['darwin'] and channel in ['nightly', 'dev', 'beta', 'release']):
        return '{CB2150F2-595F-4633-891A-E39720CE0531}'
    elif channel in ['beta']:
        return '{103BD053-949B-43A8-9120-2E424887DE11}'
    elif channel in ['release']:
        return '{AFE6A462-C574-4B8A-AF43-4CC60DF4563B}'
    elif channel in ['nightly']:
        return '{C6CB981E-DB30-4876-8639-109F8933582C}'


def get_app_info(appinfo, args):
    '''
    Returns a dict with all the info about the omaha app that we will need
    to perform the upload
    '''

    changelog_url = 'https://github.com/brave/brave-browser/blob/master/CHANGELOG_DESKTOP.md'

    if args.version:
        version_values = args.version.split('.')
        chrome_major = version_values[0]
        brave_version = '.'.join(version_values[1:3])
        version = args.version
    else:
        chrome_major = get_chrome_version().split('.')[0]
        brave_version = get_upload_version()
        version = chrome_major + '.' + brave_version
        version_values = version.split('.')

    # The Sparkle CFBundleVersion is no longer tied to the Chrome version,
    # instead we derive it from the package.json['version'] string. The 2nd
    # digit is adjusted, and then we utilize that combined with the 3rd digit as
    # the CFBundleVersion. (This is also used in build/mac/tweak_info_plist.py)
    if int(version_values[1]) >= 1:
        adjusted_minor = int(version_values[2]) + (100 * int(version_values[1]))
    else:
        # Fall back to returning the actual minor value
        adjusted_minor = int(version_values[2])

    appinfo['platform'] = 'darwin' if 'darwin' in args.platform else 'win32'
    appinfo['arch'] = 'ia32' if 'win32' in args.platform else 'x64'
    appinfo['appguid'] = get_appguid(release_channel(), appinfo['platform'])
    appinfo['channel'] = release_channel()
    appinfo['platform_id'] = get_platform_id(appinfo['platform'])
    appinfo['internal'] = args.internal
    appinfo['full'] = args.full
    appinfo['previous'] = args.previous
    if appinfo['platform'] in 'win32':
        # By default enable the win32 version on upload
        appinfo['is_enabled'] = True
        # The win32 version is the equivalent of the 'short_version' on darwin
        appinfo['version'] = version
    if appinfo['platform'] in 'darwin':
        appinfo['short_version'] = version
        appinfo['version'] = str(adjusted_minor) + '.' + version_values[3]
    appinfo['release_notes'] = 'Release notes at <a href="{0}">{0}</a>'.format(changelog_url)

    return appinfo


def get_upload_version():
    '''
    Returns the version of brave-browser
    '''
    return get_raw_version()


def sign_update_sparkle(dmg, dsaprivpem):
    '''
    Signs the Darwin dmg and returns the base64 encoded hash.

    This replaces the functionality in:
    https://github.com/brave/Sparkle/blob/master/bin/sign_update

    Need to run the equivalent of the command:
    `$openssl dgst -sha1 -binary < "$1" | $openssl dgst -sha1 -sign "$2" | $openssl enc -base64`
    '''

    import base64
    from cryptography.hazmat.backends import default_backend
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import padding

    digest = hashes.Hash(hashes.SHA1(), backend=default_backend())
    sha1digest = None

    with open(dmg, 'rb') as dmg:
        with open(dsaprivpem, 'rb') as key:
            dmgcontent = dmg.read()
            digest.update(dmgcontent)
            sha1digest = digest.finalize()

            private_key = serialization.load_pem_private_key(key.read(), password=None,
                                                             backend=default_backend())
            signature = private_key.sign(sha1digest, hashes.SHA1())
            encoded_sign = base64.encodestring(signature)

    if sha1digest is not None:
        return encoded_sign

# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

import os

BRAVE_THIRD_PARTY_DIRS = [
    'vendor',
]

ANDROID_ONLY_PATHS = []

DESKTOP_ONLY_PATHS = []


def AddBraveCredits(prune_paths, special_cases, prune_dirs, additional_paths):
    # Exclude these specific paths from needing a README.chromium file.
    prune_paths.update([
        # Formerly external Brave code which has moved to brave-core
        # (i.e these are already covered by the Brave Browser license notice).
        os.path.join('brave', 'vendor', 'bat-native-ads'),
        os.path.join('brave', 'vendor', 'bat-native-ledger'),
        os.path.join('brave', 'vendor', 'brave-ios'),
        os.path.join('brave', 'vendor', 'brave_base'),

        # Brave overrides to third-party code, also covered by main notice.
        os.path.join('brave', 'third_party', 'android_deps'),
        os.path.join('brave', 'third_party', 'blink'),
        os.path.join('brave', 'third_party', 'libaddressinput'),

        # Build dependencies which don't end up in the binaries.
        os.path.join('brave', 'vendor', 'depot_tools'),
        os.path.join('brave', 'vendor', 'gn-project-generators')
    ])

    # Add the licensing info that would normally be in a README.chromium file.
    # This is for when we pull in external repos directly.
    special_cases.update({
        os.path.join('brave', 'vendor', 'bat-native-anonize'): {
            "Name": "bat-native-anonize",
            "URL": "https://github.com/brave-intl/bat-native-anonize",
            "License": "Apache-2.0",
            "License File": "/brave/vendor/bat-native-anonize/LICENSE.txt",
        },
        os.path.join('brave', 'vendor', 'bat-native-rapidjson'): {
            "Name": "RapidJSON",
            "URL": "https://github.com/brave-intl/bat-native-rapidjson",
            "License": "MIT",
            "License File": "/brave/vendor/bat-native-rapidjson/license.txt",
        },
        os.path.join('brave', 'vendor', 'bat-native-tweetnacl'): {
            "Name": "TweetNaCl",
            "URL": "https://github.com/brave-intl/bat-native-tweetnacl",
            "License": "MPL-2.0",
        },
        os.path.join('brave', 'vendor', 'bip39wally-core-native'): {
            "Name": "libwally-core",
            "URL": "https://github.com/brave-intl/bat-native-bip39wally-core",
            "License": "MIT",
        },
        os.path.join('brave', 'vendor', 'boto'): {
            "Name": "boto",
            "URL": "https://github.com/boto/boto",
            "License": "MIT",
        },
        os.path.join('brave', 'vendor', 'brave-extension'): {
            "Name": "Brave Only Extension",
            "URL": "https://github.com/brave/brave-extension",
            "License": "MPL-2.0",
        },
        os.path.join('brave', 'vendor', 'challenge_bypass_ristretto_ffi'): {
            "Name": "challenge-bypass-ristretto-ffi",
            "URL": "https://github.com/brave-intl/challenge-bypass-ristretto-ffi",
            "License": "MPL-2.0",
        },
        os.path.join('brave', 'vendor', 'extension-whitelist'): {
            "Name": "extension-whitelist",
            "URL": "https://github.com/brave/extension-whitelist",
            "License": "MPL-2.0",
        },
        os.path.join('brave', 'vendor', 'hashset-cpp'): {
            "Name": "Hash Set",
            "URL": "https://github.com/brave/hashset-cpp",
            "License": "MPL-2.0",
        },
        os.path.join('brave', 'vendor', 'omaha'): {
            "Name": "Omaha",
            "URL": "https://github.com/brave/omaha",
            "License": "Apache-2.0",
            "License File": "/brave/vendor/omaha/LICENSE.txt",
        },
        os.path.join('brave', 'vendor', 'python-patch'): {
            "Name": "Python Patch",
            "URL": "https://github.com/brave/python-patch",
            "License": "MIT",
            "License File": "/brave/vendor/python-patch/doc/LICENSE",
        },
        os.path.join('brave', 'vendor', 'requests'): {
            "Name": "Requests",
            "URL": "https://github.com/psf/requests",
            "License": "Apache-2.0",
        },
        os.path.join('brave', 'vendor', 'sparkle'): {
            "Name": "Sparkle",
            "URL": "https://github.com/brave/Sparkle",
            "License": "MIT",
        },
    })

    # Don't recurse into these directories looking for third-party code.
    prune_list = list(prune_dirs)
    prune_list += [
        'chromium_src',  # Brave's overrides, covered by main notice.
        'node_modules',  # See brave/third_party/npm-* instead.
        '.vscode',       # Automatically added by Visual Studio.
    ]
    prune_dirs = tuple(prune_list)

    # Look for a README.chromium file directly inside these directories.
    # This is for directories which include third-party code that isn't
    # contained under a "third_party" or "vendor" directory.
    additional_list = list(additional_paths)
    additional_list += [
        os.path.join('brave', 'components', 'brave_prochlo'),
        os.path.join('brave', 'components', 'brave_new_tab_ui', 'data'),
    ]
    additional_paths = tuple(additional_list)

    return (prune_dirs, additional_paths)


def CheckBraveMissingLicense(target_os, path, error):
    if path.startswith('brave'):
        if (target_os == 'android'):
            if path in DESKTOP_ONLY_PATHS:
                return  # Desktop failures are not relevant on Android.
        else:
            if path in ANDROID_ONLY_PATHS:
                return  # Android failures are not relevant on desktop.
        raise error

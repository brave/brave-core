# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

import os
import re
import subprocess

BRAVE_SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))

BRAVE_THIRD_PARTY_DIRS = [
    'vendor',
]

ANDROID_ONLY_PATHS = []

DESKTOP_ONLY_PATHS = []

_REPOSITORY_ROOT = None


def AddBraveCredits(root, prune_paths, special_cases, prune_dirs,
                    additional_paths):
    global _REPOSITORY_ROOT  # pylint: disable=global-statement
    _REPOSITORY_ROOT = root

    # Exclude these specific paths from needing a README.chromium file.
    prune_paths.update([
        # Formerly external Brave code which has moved to brave-core
        # (i.e these are already covered by the Brave Browser license notice).
        os.path.join('brave', 'vendor', 'brave-ios'),
        os.path.join('brave', 'vendor', 'brave_base'),

        # Metadata files for Rust crates are located in the subfolders of
        # brave/third_party/rust/<crate_name>/<v>, the crates themselves in
        # brave/third_party/rust/chromium_crates_io can be skipped.
        os.path.join('brave', 'third_party', 'rust', 'chromium_crates_io'),

        # Same for upstream
        os.path.join('third_party', 'rust', 'chromium_crates_io'),

        # Rust code written by Brave and under the same license as the browser.
        os.path.join('brave', 'third_party', 'rust', 'adblock_cxx'),
        os.path.join('brave', 'third_party', 'rust', 'brave_news_cxx'),
        os.path.join('brave', 'third_party', 'rust', 'brave_wallet'),
        os.path.join('brave', 'third_party', 'rust',
                     'challenge_bypass_ristretto_cxx'),
        os.path.join('brave', 'third_party', 'rust', 'constellation_cxx'),
        os.path.join('brave', 'third_party', 'rust', 'json_cxx'),
        os.path.join('brave', 'third_party', 'rust', 'filecoin_cxx'),
        os.path.join('brave', 'third_party', 'rust', 'skus'),
        os.path.join('brave', 'third_party', 'rust', 'skus_cxx'),
        os.path.join('brave', 'third_party', 'rust', 'speedreader'),
        os.path.join('brave', 'third_party', 'rust', 'speedreader_ffi'),

        # Rust crates that are references to upstream crates and should have
        # licenses in upstream //third_party/rust.
        os.path.join('brave', 'third_party', 'rust', 'aho_corasick'),
        os.path.join('brave', 'third_party', 'rust', 'anyhow'),
        os.path.join('brave', 'third_party', 'rust', 'base64'),
        os.path.join('brave', 'third_party', 'rust', 'cfg_if'),
        os.path.join('brave', 'third_party', 'rust', 'cxx'),
        os.path.join('brave', 'third_party', 'rust', 'cxxbridge_flags'),
        os.path.join('brave', 'third_party', 'rust', 'cxxbridge_macro'),
        os.path.join('brave', 'third_party', 'rust', 'getrandom', 'v0_2'),
        os.path.join('brave', 'third_party', 'rust', 'hex'),
        os.path.join('brave', 'third_party', 'rust', 'itoa', 'v1'),
        os.path.join('brave', 'third_party', 'rust', 'lazy_static'),
        os.path.join('brave', 'third_party', 'rust', 'libc'),
        os.path.join('brave', 'third_party', 'rust', 'log'),
        os.path.join('brave', 'third_party', 'rust', 'memchr'),
        os.path.join('brave', 'third_party', 'rust', 'ppv_lite86'),
        os.path.join('brave', 'third_party', 'rust', 'proc_macro2'),
        os.path.join('brave', 'third_party', 'rust', 'quote'),
        os.path.join('brave', 'third_party', 'rust', 'rand', 'v0_8'),
        os.path.join('brave', 'third_party', 'rust', 'rand_core', 'v0_6'),
        os.path.join('brave', 'third_party', 'rust', 'regex'),
        os.path.join('brave', 'third_party', 'rust', 'regex_automata'),
        os.path.join('brave', 'third_party', 'rust', 'regex_syntax'),
        os.path.join('brave', 'third_party', 'rust', 'ryu'),
        os.path.join('brave', 'third_party', 'rust', 'serde'),
        os.path.join('brave', 'third_party', 'rust', 'serde_json'),
        os.path.join('brave', 'third_party', 'rust', 'static_assertions'),
        os.path.join('brave', 'third_party', 'rust', 'syn'),
        os.path.join('brave', 'third_party', 'rust', 'unicode_ident'),
        os.path.join('brave', 'third_party', 'rust', 'winapi'),

        # Rust crates that are downloaded but not used (due to Cargo.toml
        # misconfigurations in other crates).
        os.path.join('brave', 'third_party', 'rust', 'valuable'),
        os.path.join('brave', 'third_party', 'rust',
                     'windows_aarch64_gnullvm'),
        os.path.join('brave', 'third_party', 'rust', 'windows_i686_gnu'),
        os.path.join('brave', 'third_party', 'rust', 'windows_x86_64_gnu'),
        os.path.join('brave', 'third_party', 'rust', 'windows_x86_64_gnullvm'),

        # No third-party code directly under android_deps. It's all under
        # android_deps/libs instead and it's special-cased further down.
        os.path.join('brave', 'third_party', 'android_deps'),

        # No third-party code directly under ios_deps.
        os.path.join('brave', 'third_party', 'ios_deps'),

        # Brave overrides to third-party code, also covered by main notice.
        os.path.join('brave', 'third_party', 'blink'),
        os.path.join('brave', 'third_party', 'libaddressinput'),
        os.path.join('brave', 'third_party', 'tflite'),
        os.path.join('brave', 'third_party', 'tflite_support'),
        os.path.join('brave', 'patches', 'third_party'),
        os.path.join('brave', 'third_party', 'polymer'),
        os.path.join('brave', 'third_party', 'lit'),

        # Dependencies that are already in brave-core, and whose notices
        # therefore do not need to be repeated.
        os.path.join('brave', 'vendor', 'omaha', 'omaha', 'third_party',
                     'chrome'),
        os.path.join('brave', 'vendor', 'omaha', 'third_party', 'libzip'),
        os.path.join('brave', 'vendor', 'omaha', 'third_party', 'lzma'),
        os.path.join('brave', 'vendor', 'omaha', 'third_party', 'zlib'),

        # Dependencies already mentioned in the main breakpad notice.
        os.path.join('brave', 'vendor', 'omaha', 'third_party', 'breakpad',
                     'src', 'third_party', 'curl'),
        os.path.join('brave', 'vendor', 'omaha', 'third_party', 'breakpad',
                     'src', 'third_party', 'libdisasm'),
        os.path.join('brave', 'vendor', 'omaha', 'third_party', 'breakpad',
                     'src', 'third_party', 'linux'),
        os.path.join('brave', 'vendor', 'omaha', 'third_party', 'breakpad',
                     'src', 'third_party', 'mac_headers'),

        # Essentially empty directories that should be cleaned up upstream.
        os.path.join('brave', 'vendor', 'omaha', 'omaha', 'third_party',
                     'hashlib'),

        # No licensing information in recursive dependency. This should be
        # added upstream.
        os.path.join('brave', 'vendor', 'omaha', 'omaha', 'third_party',
                     'smartany'),

        # Build dependencies which don't end up in the binaries.
        os.path.join('brave', 'vendor', 'depot_tools'),
        os.path.join('brave', 'vendor', 'gn-project-generators'),
        os.path.join('brave', 'vendor', 'omaha', 'omaha', 'scons-out'),
        os.path.join('brave', 'third_party', 'libdmg-hfsplus'),
    ])

    # Add the licensing info that would normally be in a README.chromium file.
    # This is for when we pull in external repos directly.
    special_cases.update({
        os.path.join('brave', 'vendor', 'bat-native-tweetnacl'): {
            "Name": "TweetNaCl",
            "URL": "https://github.com/brave-intl/bat-native-tweetnacl",
            "License": "MPL-2.0",
        },
        os.path.join('brave', 'third_party', 'bip39wally-core-native'): {
            "Name": "libwally-core",
            "URL": "https://github.com/brave-intl/bat-native-bip39wally-core",
            "License": "MIT",
        },
        os.path.join('brave', 'third_party', 'rust', 'futures_retry', 'v0_5'): {
            "Name": "futures-retry",
            "URL": "https://crates.io/crates/futures-retry",
            "License": "Apache-2.0",
        },
        os.path.join('brave', 'vendor', 'brave-extension'): {
            "Name": "Brave Only Extension",
            "URL": "https://github.com/brave/brave-extension",
            "License": "MPL-2.0",
        },
        os.path.join('brave', 'vendor', 'web-discovery-project'): {
            "Name": "Web Discovery Project",
            "URL": "https://github.com/brave/web-discovery-project",
            "License": "MPL-2.0",
        },
        os.path.join('brave', 'vendor', 'omaha'): {
            "Name": "Omaha",
            "URL": "https://github.com/brave/omaha",
            "License": "Apache-2.0",
            "License File": ["/brave/vendor/omaha/LICENSE.txt"],
        },
        os.path.join('brave', 'vendor', 'omaha', 'third_party', 'breakpad'): {
            "Name": "Breakpad",
            "URL": "https://chromium.googlesource.com/breakpad/breakpad",
            "License File": [
                "/brave/vendor/omaha/third_party/breakpad/LICENSE"
            ],
        },
        # Unclear why, but presumbit wants this line formatted this way, while
        # at the same time complaining it's too long when it is formatted so.
        os.path.join('brave', 'vendor', 'omaha', 'third_party', 'breakpad', 'src', 'third_party', 'musl'): {  # pylint: disable=line-too-long
            "Name": "musl",
            "URL": "https://musl.libc.org/",
            "License File": [
                "/brave/vendor/omaha/third_party/breakpad/src/third_party/"
                "musl/COPYRIGHT"
            ],
        },
        os.path.join('brave', 'vendor', 'omaha', 'third_party', 'googletest'): {
            "Name": "GoogleTest",
            "URL": "https://github.com/google/googletest",
            "License": "BSD",
            "License File": [
                "/brave/vendor/omaha/third_party/googletest/LICENSE"
            ],
        },
        os.path.join('brave', 'third_party', 'rapidjson'): {
            "Name": "RapidJSON",
            "URL": "https://github.com/Tencent/rapidjson",
            "License": "MIT",
            "License File": ["/brave/third_party/rapidjson/src/license.txt"],
        },
        os.path.join('brave', 'third_party', 'reclient_configs'): {
            "Name": "reclient-configs",
            "URL": "https://github.com/EngFlow/reclient-configs",
            "License": "Apache-2.0",
            "License File": ["/brave/third_party/reclient_configs/src/LICENSE"],
        },
        os.path.join('brave', 'vendor', 'python-patch'): {
            "Name": "Python Patch",
            "URL": "https://github.com/brave/python-patch",
            "License": "MIT",
            "License File": ["/brave/vendor/python-patch/doc/LICENSE"],
        },
        os.path.join('brave', 'vendor', 'sparkle'): {
            "Name": "Sparkle",
            "URL": "https://github.com/brave/Sparkle",
            "License": "MIT",
        },
        os.path.join('brave', 'third_party', 'cryptography'): {
            "Name": "cryptography",
            "URL": "https://cryptography.io",
            "License": "Apache-2.0",
            "License File": ["/brave/common/licenses/Apache-2.0"],
        },
        os.path.join('brave', 'third_party', 'macholib'): {
            "Name": "macholib",
            "URL": "https://github.com/ronaldoussoren/macholib",
            "License": "MIT",
        },
    })

    # Don't recurse into these directories looking for third-party code.
    prune_list = list(prune_dirs)
    prune_list += [
        'chromium_src',  # Brave's overrides, covered by main notice.
        'node_modules',  # See brave/third_party/npm-* instead.
        '.vscode',  # Automatically added by Visual Studio.
    ]
    prune_dirs = tuple(prune_list)

    # Look for a README.chromium file directly inside these directories.
    # This is for directories which include third-party code that isn't
    # contained under a "third_party" or "vendor" directory.
    additional_list = list(additional_paths)
    additional_list += [
        os.path.join('brave', 'components', 'brave_new_tab_ui', 'data'),
        os.path.join('brave', 'browser', 'brave_vpn', 'win',
                     'brave_vpn_wireguard_service'),
        os.path.join('brave', 'components', 'filecoin'),
        os.path.join('brave', 'android', 'java', 'org', 'chromium', 'chrome',
                     'browser', 'util'),
    ]

    # Add all Android libraries since they're not directly contained
    # within a third_party directory.
    android_libs = os.path.join('brave', 'third_party', 'android_deps', 'libs')
    for _, dirs, _ in os.walk(os.path.join(root, android_libs)):
        for dirpath in dirs:
            dirname = os.path.basename(dirpath)
            additional_list += [os.path.join(android_libs, dirname)]

    # Add all iOS libraries since they're not directly contained
    # within a third_party directory. iOS deps will never be nested
    ios_deps = os.path.join('brave', 'third_party', 'ios_deps')
    for dirname in os.listdir(os.path.join(root, ios_deps)):
        if not os.path.isdir(os.path.join(root, ios_deps, dirname)):
            continue
        additional_list += [os.path.join(ios_deps, dirname)]

    additional_paths = tuple(additional_list)

    return (prune_dirs, additional_paths)


def CheckBraveMissingLicense(target_os, path, error):
    if path.startswith('brave'):
        # brave/third_party/rust itself doesn't need to have a license, but
        # all subfolders in it should.
        if path == os.path.join('brave', 'third_party', 'rust'):
            return
        output = subprocess.check_output(
            [
                'git', 'status', '-z',
                os.path.join(os.path.relpath(path, 'brave'), 'LICENSE')
            ],
            cwd=os.path.abspath(os.path.join(BRAVE_SCRIPT_PATH,
                                             os.pardir))).decode("utf-8")
        if output.startswith('??'):
            return  # Ignore untracked files

        if target_os == 'android':
            if path in DESKTOP_ONLY_PATHS:
                return  # Desktop failures are not relevant on Android.
        else:
            if path in ANDROID_ONLY_PATHS:
                return  # Android failures are not relevant on desktop.
        if not ContainsFiles(os.path.join(_REPOSITORY_ROOT, path)):
            return  # Empty directories do not require license.
        print('\nERROR: missing license information in %s\n'
              "If this is code you added, then you'll have to add the required "
              "metadata.\nIf the path that's mentioned isn't something you "
              "added, then you probably just need to remove that obsolete path "
              "from your local checkout.\n" % path)
        raise error


def ContainsFiles(path):
    assert os.path.exists(path), f'{path} not found'

    def reraise(e):
        raise e

    for _, _, filenames in os.walk(path, onerror=reraise):
        if filenames:
            return True

    return False


def IsBraveRustCrate(path):
    if not path:
        return False
    sep = re.escape(os.path.sep)
    path_regex = re.compile(
        r'''^(brave{sep})?third_party{sep}rust{sep}{nonsep}+'''.format(
            sep=sep, nonsep=f'[^{sep}]'))
    return path_regex.fullmatch(path) != None


def ReportBraveIncompleteMetadataFile(path):
    if path.startswith('brave'):
        # If there's no LICENSE file in the third party downloaded code, then
        # we place LICENSE file next to README.chromium. This file cannot be
        # added as a 'License File' into the README.chromium metadata, though,
        # so this third party code won't be added into the credits page.
        added_license_file = os.path.join(_REPOSITORY_ROOT,
                                          os.path.join(path, 'LICENSE'))
        if (os.path.isfile(added_license_file)):
            return

        raise ValueError(
            '\n\nMETADATA ERROR: missing required fields in README.chromium in '
            f'{path}\nIf this is code you added, then these metadata fields '
            'in generated README.chromium are required: Name, URL, Shipped, '
            'License File. You can fix it in one of these ways:'
            '\n* If this is a Rust crate that you added and it has a custom '
            'License File name, you can configure the custom name in '
            '//brave/third_party/rust/chromium_crates_io/gnrt_config.toml.'
            '\n* If this is a Rust crate that does not have a LICENSE file, '
            'then either add LICENSE file in the same folder where the '
            'README.chromium file is if the code has MIT license, or configure '
            'the custom relative path to shared Apache-2.0 or MPL-2.0 licenses '
            '(in //brave/commone/licenses) in '
            '//brave/third_party/rust/chromium_crates_io/gnrt_config.toml'
            '\n* If this is a Rust crate that is just a reference to an '
            'upstream crate, then add it as an exception to prune_paths in '
            '//brave/script/brave_license_helper.py.\n')

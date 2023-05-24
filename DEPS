use_relative_paths = True

gclient_gn_args_file = 'build/config/gclient_args.gni'
gclient_gn_args = [
  'brave_rust_version'
]

vars = {
  'brave_rust_version': Str('1.67.1'),
  'download_prebuilt_sparkle': True,
}

deps = {
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/brave/python-patch@d8880110be6554686bc08261766538c2926d4e82",
  "vendor/omaha": {
    "url": "https://github.com/brave/omaha.git@826cb43721b58d6da989c8486af9bfe0edb99e22",
    "condition": "checkout_win",
  },
  "vendor/sparkle": {
    "url": "https://github.com/brave/Sparkle.git@8721f93f694244f9ff41fe975a92617ac5f63f9a",
    "condition": "checkout_mac",
  },
  "vendor/bat-native-tweetnacl": "https://github.com/brave-intl/bat-native-tweetnacl.git@800f9d40b7409239ff192e0be634764e747c7a75",
  "vendor/gn-project-generators": "https://github.com/brave/gn-project-generators.git@b76e14b162aa0ce40f11920ec94bfc12da29e5d0",
  "vendor/web-discovery-project": "https://github.com/brave/web-discovery-project@212de4d3b64d8a9609ffa3143329590818deb3c1",
  "third_party/bip39wally-core-native": "https://github.com/brave-intl/bat-native-bip39wally-core.git@0d3a8713a2b388d2156fe49a70ef3f7cdb44b190",
  "third_party/ethash/src": "https://github.com/chfast/ethash.git@e4a15c3d76dc09392c7efd3e30d84ee3b871e9ce",
  "third_party/bitcoin-core/src": "https://github.com/bitcoin/bitcoin.git@95ea54ba089610019a74c1176a2c7c0dba144b1c",
  "third_party/argon2/src": "https://github.com/P-H-C/phc-winner-argon2.git@62358ba2123abd17fccf2a108a301d4b52c01a7c",
  "third_party/rapidjson/src": "https://github.com/Tencent/rapidjson.git@06d58b9e848c650114556a23294d0b6440078c61",
  'third_party/android_deps/libs/com_google_android_play_core': {
      'packages': [
          {
              'package': 'chromium/third_party/android_deps/libs/com_google_android_play_core',
              'version': 'version:2@1.10.0.cr1',
          },
      ],
      'condition': 'checkout_android',
      'dep_type': 'cipd',
  },
  "third_party/playlist_component/src": "https://github.com/brave/playlist-component.git@5434730bf7342f1ba5c057f1640882bb38604a85",
  "third_party/constellation/crate": "https://github.com/brave/constellation.git@b6b8396abd98cc87a187e051c32a291c9faa43f7",
}

recursedeps = [
  'vendor/omaha'
]

hooks = [
  {
    'name': 'bootstrap',
    'pattern': '.',
    'action': ['python', 'script/bootstrap.py'],
  },
  {
    # Download hermetic xcode for goma
    'name': 'download_hermetic_xcode',
    'pattern': '.',
    'condition': 'checkout_mac or checkout_ios',
    'action': ['vpython3', 'build/mac/download_hermetic_xcode.py'],
  },
  {
    'name': 'download_sparkle',
    'pattern': '.',
    'condition': 'checkout_mac and download_prebuilt_sparkle',
    'action': ['vpython3', 'build/mac/download_sparkle.py', '1.24.3'],
  },
  {
    'name': 'download_rust_deps',
    'pattern': '.',
    'action': [
      'vpython3', 'script/download_rust_deps.py',
      '--rust_version={brave_rust_version}',
      '--checkout_android={checkout_android}',
      '--checkout_ios={checkout_ios}',
      '--checkout_linux={checkout_linux}',
      '--checkout_mac={checkout_mac}',
      '--checkout_win={checkout_win}',
    ]
  },
  {
    # Install Web Discovery Project dependencies for Windows, Linux, and macOS
    'name': 'web_discovery_project_npm_deps',
    'pattern': '.',
    'condition': 'checkout_linux or checkout_mac or checkout_win',
    'action': ['vpython3', 'script/web_discovery_project.py', '--install'],
  },
  {
    'name': 'generate_licenses',
    'pattern': '.',
    'action': ['vpython3', 'script/generate_licenses.py'],
  },
]

include_rules = [
  # Everybody can use some things.
  "+brave_base",
  "+crypto",
  "+net",
  "+sql",
  "+ui/base",

  "-chrome",
  "-brave/app",
  "-brave/browser",
  "-brave/common",
  "-brave/renderer",
  "-brave/services",
  "-ios",
  "-brave/third_party/bitcoin-core",
  "-brave/third_party/argon2",
]

# Temporary workaround for massive nummber of incorrect test includes
specific_include_rules = {
  ".*test.*(\.cc|\.mm|\.h)": [
    "+bat",
    "+brave",
    "+chrome",
    "+components",
    "+content",
    "+extensions",
    "+mojo",
    "+services",
    "+third_party",
  ],
}

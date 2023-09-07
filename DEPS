use_relative_paths = True

vars = {
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
  "vendor/web-discovery-project": "https://github.com/brave/web-discovery-project@0c281cbdba6fe9c9bbe1d96095e20f857268ddbf",
  "third_party/bip39wally-core-native": "https://github.com/brave-intl/bat-native-bip39wally-core.git@0d3a8713a2b388d2156fe49a70ef3f7cdb44b190",
  "third_party/ethash/src": "https://github.com/chfast/ethash.git@e4a15c3d76dc09392c7efd3e30d84ee3b871e9ce",
  "third_party/bitcoin-core/src": "https://github.com/bitcoin/bitcoin.git@95ea54ba089610019a74c1176a2c7c0dba144b1c",
  "third_party/argon2/src": "https://github.com/P-H-C/phc-winner-argon2.git@62358ba2123abd17fccf2a108a301d4b52c01a7c",
  "third_party/rapidjson/src": "https://github.com/Tencent/rapidjson.git@06d58b9e848c650114556a23294d0b6440078c61",
  'third_party/android_deps/libs/com_google_android_play_core': {
      'packages': [
          {
              'package': 'chromium/third_party/android_deps/libs/com_google_android_play_core',
              'version': 'version:2@1.10.3.cr1',
          },
      ],
      'condition': 'checkout_android',
      'dep_type': 'cipd',
  },
  "third_party/playlist_component/src": "https://github.com/brave/playlist-component.git@5434730bf7342f1ba5c057f1640882bb38604a85",
  "third_party/rust/star_constellation/v0_2/crate": "https://github.com/brave/constellation.git@db575edec12509ce1bda6afe68bb58e538a21d3a",
  "third_party/rust/challenge_bypass_ristretto/v1/crate": "https://github.com/brave-intl/challenge-bypass-ristretto.git@a1da4641734adc8312215b38a8221962d2c8e045",
  "third_party/rust/futures_retry/v0_5/crate": "https://github.com/brave-intl/futures-retry.git@2aaaafbc3d394661534d4dbd14159d164243c20e",
  "third_party/rust/kuchiki/v0_8/crate": "https://github.com/brave/kuchiki.git@589eadca2c1d06ddda2919354590bfe1ace88a43",
  "third_party/macholib": {
    "url": "https://github.com/ronaldoussoren/macholib.git@36a6777ccd0891c5d1b44ba885573d7c90740015",
    "condition": "checkout_mac",
  },
}

recursedeps = [
  'vendor/omaha'
]

hooks = [
  {
    'name': 'bootstrap',
    'pattern': '.',
    'action': ['vpython3', 'script/bootstrap.py'],
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
    'name': 'update_pip',
    'pattern': '.',
    # Required for download_cryptography below. Specifically, newer versions of
    # pip are required for obtaining binary wheels on Arm64 macOS.
    'action': ['python3', '-m', 'pip', '-q', '--disable-pip-version-check', 'install', '-U', '--no-warn-script-location', 'pip'],
  },
  {
    'name': 'download_cryptography',
    'pattern': '.',
    # We don't include cryptography as a DEP because building it from source is
    # difficult. We pin to a version >=37.0.2 and <38.0.0 to avoid an
    # incompatibility with our pyOpenSSL version on Android. See:
    # https://github.com/pyca/cryptography/issues/7126.
    # We use python3 instead of vpython3 for two reasons: First, our GN actions
    # are run with python3, so this environment mirrors the one in which
    # cryptography will be used. Second, we cannot update pip in vpython3 on at
    # least macOS due to permission issues.
    'action': ['python3', '-m', 'pip', '-q', '--disable-pip-version-check', 'install', '-U', '-t', 'third_party/cryptography', '--only-binary', 'cryptography', 'cryptography==37.0.4'],
  },
  {
    'name': 'wireguard_nt',
    'pattern': '.',
    'condition': 'checkout_win',
    'action': ['vpython3', 'build/win/download_brave_vpn_wireguard_binaries.py', '0.10.1', 'brave-vpn-wireguard-nt-dlls'],
  },
  {
    'name': 'wireguard_tunnel',
    'pattern': '.',
    'condition': 'checkout_win',
    'action': ['vpython3', 'build/win/download_brave_vpn_wireguard_binaries.py', 'v0.5.3', 'brave-vpn-wireguard-tunnel-dlls'],
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
  {
    # Overwrite Chromium's LASTCHANGE using the latest Brave version commit.
    'name': 'brave_lastchange',
    'pattern': '.',
    'action': ['python3', '../build/util/lastchange.py',
               '--output', '../build/util/LASTCHANGE',
               '--source-dir', '.',
               '--filter', '^[0-9]\{{1,\}}\.[0-9]\{{1,\}}\.[0-9]\{{1,\}}$'],
  },
]

include_rules = [
  #Everybody can use some things.
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

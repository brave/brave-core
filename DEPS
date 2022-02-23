use_relative_paths = True

deps = {
  "vendor/extension-whitelist": "https://github.com/brave/extension-whitelist.git@c29f81bf84d1cccae2181347689c336576d2c8c4",
  "vendor/hashset-cpp": "https://github.com/brave/hashset-cpp.git@6eab0271d014ff09bd9f38abe1e0c117e13e9aa9",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/brave/python-patch@d8880110be6554686bc08261766538c2926d4e82",
  "vendor/omaha":  "https://github.com/brave/omaha.git@adcf41919ce8e86e7b2417ab9f53489a9c7ad9ab",
  "vendor/sparkle": "https://github.com/brave/Sparkle.git@57fb153bea4c71ed10102d50e68ead89ca483b49",
  "vendor/bat-native-rapidjson": "https://github.com/brave-intl/bat-native-rapidjson.git@60b7e4574cebdd79f441bdd6f0f3ab469fd7e04c",
  "vendor/bip39wally-core-native": "https://github.com/brave-intl/bat-native-bip39wally-core.git@0d3a8713a2b388d2156fe49a70ef3f7cdb44b190",
  "vendor/bat-native-anonize": "https://github.com/brave-intl/bat-native-anonize.git@e3742ba3e8942eea9e4755d91532491871bd3116",
  "vendor/bat-native-tweetnacl": "https://github.com/brave-intl/bat-native-tweetnacl.git@800f9d40b7409239ff192e0be634764e747c7a75",
  "vendor/challenge_bypass_ristretto_ffi": "https://github.com/brave-intl/challenge-bypass-ristretto-ffi.git@c5d6d74fdbee467732bcb6b9084514839e656286",
  "vendor/gn-project-generators": "https://github.com/brave/gn-project-generators.git@b76e14b162aa0ce40f11920ec94bfc12da29e5d0",
  "vendor/web-discovery-project": "https://github.com/brave/web-discovery-project@d3e955670e6723ca4af9dc0ce921b955f72887e4",
  "third_party/ethash/src": "https://github.com/chfast/ethash.git@e4a15c3d76dc09392c7efd3e30d84ee3b871e9ce",
  "third_party/bitcoin-core/src": "https://github.com/bitcoin/bitcoin.git@95ea54ba089610019a74c1176a2c7c0dba144b1c",
  "third_party/argon2/src": "https://github.com/P-H-C/phc-winner-argon2.git@62358ba2123abd17fccf2a108a301d4b52c01a7c",
  "third_party/boost/config": "https://github.com/boostorg/config.git@e108255ffb5d2557ed3398b3fc575a2e9fd434cc",
  "third_party/boost/multiprecision": "https://github.com/boostorg/multiprecision.git@db0bb4d2d2611605b7ce97220fb9973fd7e072c9",
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
    # Download rust deps if necessary for Android
    'name': 'download_rust_deps',
    'pattern': '.',
    'condition': 'checkout_android',
    'action': ['vpython3', 'script/download_rust_deps.py',
        '--platform', 'android'],
  },
  {
    # Download rust deps if necessary for iOS
    'name': 'download_rust_deps',
    'pattern': '.',
    'condition': 'checkout_ios',
    'action': ['vpython3', 'script/download_rust_deps.py',
        '--platform', 'ios'],
  },
  {
    # Download rust deps if necessary for Windows, Linux, and macOS
    'name': 'download_rust_deps',
    'pattern': '.',
    'condition': 'not checkout_android and not checkout_ios',
    'action': ['vpython3', 'script/download_rust_deps.py'],
  },
  {
    # Install Web Discovery Project dependencies for Windows, Linux, and macOS
    'name': 'web_discovery_project_npm_deps',
    'pattern': '.',
    'condition': 'not checkout_android and not checkout_ios',
    'action': ['vpython3', 'script/web_discovery_project.py', '--install'],
  },
  {
    'name': 'hermetic_xcode',
    'pattern': '.',
    'condition': 'checkout_mac',
    'action': ['vpython3', 'script/hermetic_xcode.py'],
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

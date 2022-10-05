use_relative_paths = True

deps = {
  "vendor/extension-whitelist": "https://github.com/brave/extension-whitelist.git@7f5d63711561bbc6b89dfa1c18f4d8d51df8c6d8",
  "vendor/hashset-cpp": "https://github.com/brave/hashset-cpp.git@6eab0271d014ff09bd9f38abe1e0c117e13e9aa9",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/brave/python-patch@d8880110be6554686bc08261766538c2926d4e82",
  "vendor/omaha": {
    "url": "https://github.com/brave/omaha.git@2ecd0b965df89efe5fefab13867f0740c973906c",
    "condition": "checkout_win",
  },
  "vendor/sparkle": {
    "url": "https://github.com/brave/Sparkle.git@f69ba7b7fae7dda475ba6cda12fba8d72270478d",
    "condition": "checkout_mac",
  },
  "vendor/bat-native-rapidjson": "https://github.com/brave-intl/bat-native-rapidjson.git@60b7e4574cebdd79f441bdd6f0f3ab469fd7e04c",
  "vendor/bip39wally-core-native": "https://github.com/brave-intl/bat-native-bip39wally-core.git@0d3a8713a2b388d2156fe49a70ef3f7cdb44b190",
  "vendor/bat-native-anonize": "https://github.com/brave-intl/bat-native-anonize.git@e3742ba3e8942eea9e4755d91532491871bd3116",
  "vendor/bat-native-tweetnacl": "https://github.com/brave-intl/bat-native-tweetnacl.git@800f9d40b7409239ff192e0be634764e747c7a75",
  "vendor/challenge_bypass_ristretto_ffi": "https://github.com/brave-intl/challenge-bypass-ristretto-ffi.git@419995e4a873c294f1eaefca276f8ad25328e89f",
  "vendor/gn-project-generators": "https://github.com/brave/gn-project-generators.git@b76e14b162aa0ce40f11920ec94bfc12da29e5d0",
  "vendor/web-discovery-project": "https://github.com/brave/web-discovery-project@1b7a869b9a0cf4d1a67d1d5f9ba24edc389f99f6",
  "third_party/ethash/src": "https://github.com/chfast/ethash.git@e4a15c3d76dc09392c7efd3e30d84ee3b871e9ce",
  "third_party/bitcoin-core/src": "https://github.com/bitcoin/bitcoin.git@95ea54ba089610019a74c1176a2c7c0dba144b1c",
  "third_party/argon2/src": "https://github.com/P-H-C/phc-winner-argon2.git@62358ba2123abd17fccf2a108a301d4b52c01a7c",
}

recursedeps = [
  'vendor/omaha'
]

vars = {
  'download_prebuilt_sparkle': True
}

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
    'action': ['vpython3', 'build/mac/download_sparkle.py', '1.24.2'],
  },
  {
    'name': 'download_rust_deps',
    'pattern': '.',
    'condition': 'checkout_android',
    'action': ['vpython3', 'script/download_rust_deps.py', 'android'],
  },
  {
    'name': 'download_rust_deps',
    'pattern': '.',
    'condition': 'checkout_ios',
    'action': ['vpython3', 'script/download_rust_deps.py', 'ios'],
  },
  {
    'name': 'download_rust_deps',
    'pattern': '.',
    'condition': 'checkout_win',
    'action': ['vpython3', 'script/download_rust_deps.py', 'win32'],
  },
  {
    'name': 'download_rust_deps',
    'pattern': '.',
    'condition': 'checkout_mac',
    'action': ['vpython3', 'script/download_rust_deps.py', 'darwin'],
  },
  {
    'name': 'download_rust_deps',
    'pattern': '.',
    'condition': 'checkout_linux',
    'action': ['vpython3', 'script/download_rust_deps.py', 'linux'],
  },
  {
    # Install Web Discovery Project dependencies for Windows, Linux, and macOS
    'name': 'web_discovery_project_npm_deps',
    'pattern': '.',
    'condition': 'not checkout_android and not checkout_ios',
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

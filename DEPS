use_relative_paths = True

deps = {
  "vendor/adblock_rust_ffi": "https://github.com/brave/adblock-rust-ffi.git@f836fd3beacc38feba01eda8452d5e06f302a655",
  "vendor/extension-whitelist": "https://github.com/brave/extension-whitelist.git@b4d059c73042cacf3a5e9156d4b1698e7bc18678",
  "vendor/hashset-cpp": "https://github.com/brave/hashset-cpp.git@6eab0271d014ff09bd9f38abe1e0c117e13e9aa9",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/brave/python-patch@d8880110be6554686bc08261766538c2926d4e82",
  "vendor/omaha":  "https://github.com/brave/omaha.git@c8b0a0e89fcd82b8a1fa0059f1e5a4ceafc9b29a",
  "vendor/sparkle": "https://github.com/brave/Sparkle.git@a8ba961ee53e7bf8484845ec14050b74f530e813",
  "vendor/bat-native-rapidjson": "https://github.com/brave-intl/bat-native-rapidjson.git@86aafe2ef89835ae71c9ed7c2527e3bb3000930e",
  "vendor/bip39wally-core-native": "https://github.com/brave-intl/bip39wally-core-native.git@13bb40a215248cfbdd87d0a6b425c8397402e9e6",
  "vendor/bat-native-anonize": "https://github.com/brave-intl/bat-native-anonize.git@e3742ba3e8942eea9e4755d91532491871bd3116",
  "vendor/bat-native-tweetnacl": "https://github.com/brave-intl/bat-native-tweetnacl.git@800f9d40b7409239ff192e0be634764e747c7a75",
  "vendor/bat-native-usermodel": "https://github.com/brave-intl/bat-native-usermodel.git@02b8c81c94072c67fe00108feb90786e088d4d26",
  "vendor/challenge_bypass_ristretto_ffi": "https://github.com/brave-intl/challenge-bypass-ristretto-ffi.git@c396fb4eb9e9bf63b89ae5a0ec0b5f201d43c7c5",
  "vendor/gn-project-generators": "https://github.com/brave/gn-project-generators.git@b76e14b162aa0ce40f11920ec94bfc12da29e5d0",
}

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
    'action': ['vpython3', 'script/download_rust_deps.py', '--platform', 'android'],
  },
  {
    # Download rust deps if necessary for macOS, Windows and Linux
    'name': 'download_rust_deps',
    'pattern': '.',
    'condition': 'not checkout_android',
    'action': ['vpython3', 'script/download_rust_deps.py'],
  },
  {
    'name': 'generate_licenses',
    'pattern': '.',
    'action': ['python', 'script/generate_licenses.py'],
  },
]

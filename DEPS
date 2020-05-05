use_relative_paths = True

deps = {
  "vendor/adblock_rust_ffi": "https://github.com/brave/adblock-rust-ffi.git@8e45290ced608da6c9b992a92b10355d9e140aae",
  "vendor/extension-whitelist": "https://github.com/brave/extension-whitelist.git@7843f62e26a23c51336330e220e9d7992680aae9",
  "vendor/hashset-cpp": "https://github.com/brave/hashset-cpp.git@6eab0271d014ff09bd9f38abe1e0c117e13e9aa9",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/brave/python-patch@d8880110be6554686bc08261766538c2926d4e82",
  "vendor/omaha":  "https://github.com/brave/omaha.git@de118d8511e4754a673d44a9f1e92d80442069e7",
  "vendor/sparkle": "https://github.com/brave/Sparkle.git@198b1f7930d3fb036eaae7a0eae34ec63ef4f479",
  "vendor/bat-native-rapidjson": "https://github.com/brave-intl/bat-native-rapidjson.git@86aafe2ef89835ae71c9ed7c2527e3bb3000930e",
  "vendor/bip39wally-core-native": "https://github.com/brave-intl/bip39wally-core-native.git@13bb40a215248cfbdd87d0a6b425c8397402e9e6",
  "vendor/bat-native-anonize": "https://github.com/brave-intl/bat-native-anonize.git@e3742ba3e8942eea9e4755d91532491871bd3116",
  "vendor/bat-native-tweetnacl": "https://github.com/brave-intl/bat-native-tweetnacl.git@800f9d40b7409239ff192e0be634764e747c7a75",
  "components/brave_sync/extension/brave-sync": {
    'url': 'https://github.com/brave/sync.git@c96a1e1fbe288b217f15250322a5c9c9377f494a',
    'condition': 'not checkout_android',
  },
  "components/brave_sync/extension/brave-sync-android": {
    'url': 'https://github.com/brave/sync.git@4098493496b19f46c33a66b8867c69ee6d1a4568',
    'condition': 'checkout_android',
  },
  "vendor/bat-native-usermodel": "https://github.com/brave-intl/bat-native-usermodel.git@45e32155af9897dbe1d5534dd36697ec4728bb75",
  "vendor/challenge_bypass_ristretto_ffi": "https://github.com/brave-intl/challenge-bypass-ristretto-ffi.git@c396fb4eb9e9bf63b89ae5a0ec0b5f201d43c7c5",
}

hooks = [
  {
    'name': 'bootstrap',
    'pattern': '.',
    'action': ['python', 'src/brave/script/bootstrap.py'],
  },
  {
    # Download rust deps if necessary for Android
    'name': 'download_rust_deps',
    'pattern': '.',
    'condition': 'checkout_android',
    'action': ['vpython3', 'src/brave/script/download_rust_deps.py', '--platform', 'android'],
  },
  {
    # Download rust deps if necessary for macOS, Windows and Linux
    'name': 'download_rust_deps',
    'pattern': '.',
    'condition': 'not checkout_android',
    'action': ['vpython3', 'src/brave/script/download_rust_deps.py'],
  },
  {
    # Build brave-sync
    'name': 'build_brave_sync',
    'pattern': '.',
    'action': ['python', 'src/brave/script/build-simple-js-bundle.py', '--repo_dir_path', 'src/brave/components/brave_sync/extension/brave-sync'],
    'condition': 'not checkout_android',
  },
  {
    # Build brave-sync android
    'name': 'build_brave_sync',
    'pattern': '.',
    'action': ['python', 'src/brave/script/build-simple-js-bundle.py', '--repo_dir_path', 'src/brave/components/brave_sync/extension/brave-sync-android'],
    'condition': 'checkout_android',
  }
]

use_relative_paths = True

deps = {
  "vendor/adblock_rust_ffi": "https://github.com/brave/adblock-rust-ffi.git@2360813922ceed63e3798737a268858c11e24d80",
  "vendor/autoplay-whitelist": "https://github.com/brave/autoplay-whitelist.git@ea527a4d36051daedb34421e129c98eda06cb5d3",
  "vendor/extension-whitelist": "https://github.com/brave/extension-whitelist.git@7843f62e26a23c51336330e220e9d7992680aae9",
  "vendor/hashset-cpp": "https://github.com/brave/hashset-cpp.git@6eab0271d014ff09bd9f38abe1e0c117e13e9aa9",
  "vendor/bloom-filter-cpp": "https://github.com/brave/bloom-filter-cpp.git@9be5c63b14e094156e00c8b28f205e7794f0b92c",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/brave/python-patch@d8880110be6554686bc08261766538c2926d4e82",
  "vendor/omaha":  "https://github.com/brave/omaha.git@de118d8511e4754a673d44a9f1e92d80442069e7",
  "vendor/sparkle": "https://github.com/brave/Sparkle.git@c0759cce415d7c0feae45005c8a013b1898711f0",
  "vendor/bat-native-rapidjson": "https://github.com/brave-intl/bat-native-rapidjson.git@86aafe2ef89835ae71c9ed7c2527e3bb3000930e",
  "vendor/bip39wally-core-native": "https://github.com/brave-intl/bip39wally-core-native.git@13bb40a215248cfbdd87d0a6b425c8397402e9e6",
  "vendor/bat-native-anonize": "https://github.com/brave-intl/bat-native-anonize.git@e3742ba3e8942eea9e4755d91532491871bd3116",
  "vendor/bat-native-tweetnacl": "https://github.com/brave-intl/bat-native-tweetnacl.git@dd0c535898a645b84d6f3372b393bdad6746108c",
  "components/brave_sync/extension/brave-sync": "https://github.com/brave/sync.git@e50a5810c12170ed0e9a9d2c9d69a5e05a77f837",
  "vendor/bat-native-usermodel": "https://github.com/brave-intl/bat-native-usermodel.git@a82acda22d8cb255d86ee28734efb8ad886e9a49",
  "vendor/challenge_bypass_ristretto_ffi": "https://github.com/brave-intl/challenge-bypass-ristretto-ffi.git@f88d942ddfaf61a4a6703355a77c4ef71bc95c35",
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
    'action': ['python', 'src/brave/script/download_rust_deps.py', '--platform', 'android'],
  },
  {
    # Download rust deps if necessary for macOS, Windows and Linux
    'name': 'download_rust_deps',
    'pattern': '.',
    'condition': 'not checkout_android',
    'action': ['python', 'src/brave/script/download_rust_deps.py'],
  },
  {
    # Build brave-sync
    'name': 'build_brave_sync',
    'pattern': '.',
    'action': ['python', 'src/brave/script/build-simple-js-bundle.py', '--repo_dir_path', 'src/brave/components/brave_sync/extension/brave-sync'],
  }
]

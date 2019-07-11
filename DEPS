use_relative_paths = True

deps = {
  "vendor/adblock_rust_ffi": "https://github.com/brave/adblock-rust-ffi.git@0e980ee0a63d837f3ecad666fabe1f50311baa81",
  "vendor/autoplay-whitelist": "https://github.com/brave/autoplay-whitelist.git@f39d83788cd12b883bfe6561cc7166b176ab9d9d",
  "vendor/extension-whitelist": "https://github.com/brave/extension-whitelist.git@aa74a1bc29ced176e50db9f54946f28d82dfc795",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@3de744d4698b2f8fac1f52579d39f9ad154d1ec2",
  "vendor/hashset-cpp": "https://github.com/brave/hashset-cpp.git@4b55fe39bb25bb0d8b11a43d547d75f00c6c46fb",
  "vendor/bloom-filter-cpp": "https://github.com/brave/bloom-filter-cpp.git@9be5c63b14e094156e00c8b28f205e7794f0b92c",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/brave/python-patch@d8880110be6554686bc08261766538c2926d4e82",
  "vendor/omaha":  "https://github.com/brave/omaha.git@0063d7ecfd6ac874719d42a4a1a9e45007137378",
  "vendor/sparkle": "https://github.com/brave/Sparkle.git@c0759cce415d7c0feae45005c8a013b1898711f0",
  "vendor/bat-native-rapidjson": "https://github.com/brave-intl/bat-native-rapidjson.git@86aafe2ef89835ae71c9ed7c2527e3bb3000930e",
  "vendor/bip39wally-core-native": "https://github.com/brave-intl/bip39wally-core-native.git@13bb40a215248cfbdd87d0a6b425c8397402e9e6",
  "vendor/bat-native-anonize": "https://github.com/brave-intl/bat-native-anonize.git@e3742ba3e8942eea9e4755d91532491871bd3116",
  "vendor/bat-native-tweetnacl": "https://github.com/brave-intl/bat-native-tweetnacl.git@1b4362968c8f22720bfb75af6f506d4ecc0f3116",
  "components/brave_sync/extension/brave-sync": "https://github.com/brave/sync.git@896d733ddcf5fb1a99611a0830d4824b4cb22a68",
  "components/brave_sync/extension/brave-crypto": "https://github.com/brave/crypto@83e7b37465a1ea03b149eb27d4a05993f76a9534",
  "vendor/bat-native-usermodel": "https://github.com/brave-intl/bat-native-usermodel.git@c1745b906079285fc01e3079ac49b0e65fde0f98",
  "vendor/challenge_bypass_ristretto_ffi": "https://github.com/brave-intl/challenge-bypass-ristretto-ffi.git@2c0e28f76e4b6f53947bf4faa5afd93614f96aca",
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
  },
  {
    # Build brave-crypto
    'name': 'build_brave_crypto',
    'pattern': '.',
    'action': ['python', 'src/brave/script/build-simple-js-bundle.py', '--repo_dir_path', 'src/brave/components/brave_sync/extension/brave-crypto'],
  }
]

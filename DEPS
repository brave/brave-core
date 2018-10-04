use_relative_paths = True

deps = {
  "vendor/ad-block": "https://github.com/brave/ad-block.git@a4f73962e9d852fbe3f75e3a766deb96f0d9065e",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@bb6013ff4d0a0191ba93158f2f3b30e7fb18c5f6",
  "vendor/hashset-cpp": "https://github.com/brave/hashset-cpp.git@f86b0a5752545274e32c0dbb654c3592cc131c8a",
  "vendor/bloom-filter-cpp": "https://github.com/brave/bloom-filter-cpp.git@635780bbedff137a6a83ec23871944e22069de5b",
  "vendor/brave-extension": "https://github.com/brave/brave-extension.git@86e9553a2551c34c2489b0dd197b76e494d0be79",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/svn2github/python-patch@a336a458016ced89aba90dfc3f4c8222ae3b1403",
  "vendor/sparkle": "https://github.com/brave/Sparkle.git@c0759cce415d7c0feae45005c8a013b1898711f0",
  "vendor/bat-native-ledger": "https://github.com/brave-intl/bat-native-ledger@ffb651cf1de368be1ab720c4604fe45a717493a4",
  "vendor/bat-native-rapidjson": "https://github.com/brave-intl/bat-native-rapidjson.git@86aafe2ef89835ae71c9ed7c2527e3bb3000930e",
  "vendor/bip39wally-core-native": "https://github.com/brave-intl/bip39wally-core-native.git@9b119931c702d55be994117eb505d56310720b1d",
  "vendor/bat-native-anonize": "https://github.com/brave-intl/bat-native-anonize.git@0559543f458a949b83b58035273ef7f8f1a1b111",
  "vendor/bat-native-tweetnacl": "https://github.com/brave-intl/bat-native-tweetnacl.git@1b4362968c8f22720bfb75af6f506d4ecc0f3116",
  "components/brave_sync/extension/brave-sync": "https://github.com/brave/sync.git@75c1bb10c4a54275de4fb10afd9f204cc07284a8",
  "components/brave_sync/extension/brave-crypto": "https://github.com/brave/crypto@c07866c3b9c8264195756a81112bb74b8fa90858",
}

hooks = [
  {
    'name': 'bootstrap',
    'pattern': '.',
    'action': ['python', 'src/brave/script/bootstrap.py'],
  },
  {
    # Apply patches to chromium src
    'name': 'apply_patches',
    'pattern': '.',
    'action': ['python', 'src/brave/script/apply-patches.py'],
  },
  {
    # Run npm install for brave-extension
    'name': 'init',
    'pattern': '.',
    'action': ['python', 'src/brave/script/init-brave-extension.py'],
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

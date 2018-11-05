use_relative_paths = True

deps = {
  "vendor/ad-block": "https://github.com/brave/ad-block.git@ac44a2979536c8bdf801fe0fa9ce8d9b0803cef6",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@e67738e656244f7ab6e0ed9815071ca744f5468f",
  "vendor/hashset-cpp": "https://github.com/brave/hashset-cpp.git@4b55fe39bb25bb0d8b11a43d547d75f00c6c46fb",
  "vendor/bloom-filter-cpp": "https://github.com/brave/bloom-filter-cpp.git@9be5c63b14e094156e00c8b28f205e7794f0b92c",
  "vendor/brave-extension": "https://github.com/brave/brave-extension.git@6ba9928516afe0f0041be138c11ac27b3964ae26",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/svn2github/python-patch@a336a458016ced89aba90dfc3f4c8222ae3b1403",
  "vendor/omaha":  "https://github.com/brave/omaha.git@5c633e867efafb9013c57ca830212d1ff6ea5076",
  "vendor/sparkle": "https://github.com/brave/Sparkle.git@c0759cce415d7c0feae45005c8a013b1898711f0",
  "vendor/bat-native-ledger": "https://github.com/brave-intl/bat-native-ledger@dd88d2192d6db435ab15f48fc2443564d1472552",
  "vendor/bat-native-rapidjson": "https://github.com/brave-intl/bat-native-rapidjson.git@86aafe2ef89835ae71c9ed7c2527e3bb3000930e",
  "vendor/bip39wally-core-native": "https://github.com/brave-intl/bip39wally-core-native.git@9b119931c702d55be994117eb505d56310720b1d",
  "vendor/bat-native-anonize": "https://github.com/brave-intl/bat-native-anonize.git@adeff3254bb90ccdc9699040d5a4e1cd6b8393b7",
  "vendor/bat-native-tweetnacl": "https://github.com/brave-intl/bat-native-tweetnacl.git@1b4362968c8f22720bfb75af6f506d4ecc0f3116",
  "components/brave_sync/extension/brave-sync": "https://github.com/brave/sync.git@ff9c7f3bdea37e8cc1cb08eff48123fd9add6da9",
  "components/brave_sync/extension/brave-crypto": "https://github.com/brave/crypto@c07866c3b9c8264195756a81112bb74b8fa90858",
  "vendor/bat-native-ads": "https://github.com/brave-intl/bat-native-ads.git@3dffeb6eab77ae530865628a7c5a6550dc8fe613",
  "vendor/bat-native-usermodel": "https://github.com/brave-intl/bat-native-usermodel.git@657b01814f2dc017b40d303bd468fa4938e825c4",
  "vendor/bat-native-confirmations": "https://github.com/brave-intl/bat-native-confirmations.git@0fc3a55e3a9bac4da636c0695e1bb1f93c6abe62",
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
    'condition': 'not checkout_android',
    'action': ['python', 'src/brave/script/apply-patches.py'],
  },
  {
    # Run npm install for brave-extension
    'name': 'init',
    'condition': 'not checkout_android',
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

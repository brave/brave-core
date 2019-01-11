use_relative_paths = True

deps = {
  "vendor/ad-block": "https://github.com/brave/ad-block.git@366274d84d144f514bf65bca71552d60721959e9",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@e67738e656244f7ab6e0ed9815071ca744f5468f",
  "vendor/hashset-cpp": "https://github.com/brave/hashset-cpp.git@4b55fe39bb25bb0d8b11a43d547d75f00c6c46fb",
  "vendor/bloom-filter-cpp": "https://github.com/brave/bloom-filter-cpp.git@9be5c63b14e094156e00c8b28f205e7794f0b92c",
  "vendor/brave-extension": "https://github.com/brave/brave-extension.git@038a43af5294150d48ce94a326290f70aa001cd0",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/svn2github/python-patch@a336a458016ced89aba90dfc3f4c8222ae3b1403",
  "vendor/omaha":  "https://github.com/brave/omaha.git@079c2f24e51c25da80abce4aaea7d1b6d533483c",
  "vendor/sparkle": "https://github.com/brave/Sparkle.git@c0759cce415d7c0feae45005c8a013b1898711f0",
  "vendor/bat-native-rapidjson": "https://github.com/brave-intl/bat-native-rapidjson.git@86aafe2ef89835ae71c9ed7c2527e3bb3000930e",
  "vendor/bip39wally-core-native": "https://github.com/brave-intl/bip39wally-core-native.git@9b119931c702d55be994117eb505d56310720b1d",
  "vendor/bat-native-anonize": "https://github.com/brave-intl/bat-native-anonize.git@19fb43fd58b852fd2c6e4d4c68daa99c835f5182",
  "vendor/bat-native-tweetnacl": "https://github.com/brave-intl/bat-native-tweetnacl.git@1b4362968c8f22720bfb75af6f506d4ecc0f3116",
  "components/brave_sync/extension/brave-sync": "https://github.com/brave/sync.git@56a0fe1510a31f7b26710ff7409f0ab79a67db4d",
  "components/brave_sync/extension/brave-crypto": "https://github.com/brave/crypto@7e391cec6975106fa9f686016f494cb8a782afcd",
  "vendor/bat-native-ads": "https://github.com/brave-intl/bat-native-ads.git@fd6d9859a8327975aec0c11091066b0d3387bf4f",
  "vendor/bat-native-usermodel": "https://github.com/brave-intl/bat-native-usermodel.git@c3b6111aa862c5c452c84be8a225d5f1df32b284",
  "vendor/challenge_bypass_ristretto_ffi": "https://github.com/brave-intl/challenge-bypass-ristretto-ffi.git@0a9320a061b77f7682261eb7303ddfa4fc734595",
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
    # Download rust deps if necessary
    'name': 'download_rust_deps',
    'pattern': '.',
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

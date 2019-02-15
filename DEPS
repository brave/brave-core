use_relative_paths = True

deps = {
  "vendor/ad-block": "https://github.com/brave/ad-block.git@aa0485f374365a1cc3b85de85e952d6599c4437f",
  "vendor/autoplay-whitelist": "https://github.com/brave/autoplay-whitelist.git@2893ad5059e1f6af15879fd669c08a92d5b69b63",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@e67738e656244f7ab6e0ed9815071ca744f5468f",
  "vendor/hashset-cpp": "https://github.com/brave/hashset-cpp.git@4b55fe39bb25bb0d8b11a43d547d75f00c6c46fb",
  "vendor/bloom-filter-cpp": "https://github.com/brave/bloom-filter-cpp.git@9be5c63b14e094156e00c8b28f205e7794f0b92c",
  "vendor/brave-extension": "https://github.com/brave/brave-extension.git@385e580507125b7aa91b6e34fcb8e90c58b8c96a",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/svn2github/python-patch@a336a458016ced89aba90dfc3f4c8222ae3b1403",
  "vendor/omaha":  "https://github.com/brave/omaha.git@22dec7e124881ba2c7e8f331d18d9c4dd40ed207",
  "vendor/sparkle": "https://github.com/brave/Sparkle.git@c0759cce415d7c0feae45005c8a013b1898711f0",
  "vendor/bat-native-rapidjson": "https://github.com/brave-intl/bat-native-rapidjson.git@86aafe2ef89835ae71c9ed7c2527e3bb3000930e",
  "vendor/bip39wally-core-native": "https://github.com/brave-intl/bip39wally-core-native.git@9b119931c702d55be994117eb505d56310720b1d",
  "vendor/bat-native-anonize": "https://github.com/brave-intl/bat-native-anonize.git@19fb43fd58b852fd2c6e4d4c68daa99c835f5182",
  "vendor/bat-native-tweetnacl": "https://github.com/brave-intl/bat-native-tweetnacl.git@1b4362968c8f22720bfb75af6f506d4ecc0f3116",
  "components/brave_sync/extension/brave-sync": "https://github.com/brave/sync.git@aa8e7866d3e6792c224524d5f7e5dd393c7282de",
  "components/brave_sync/extension/brave-crypto": "https://github.com/brave/crypto@7e391cec6975106fa9f686016f494cb8a782afcd",
  "vendor/bat-native-ads": "https://github.com/brave-intl/bat-native-ads.git@fd6d9859a8327975aec0c11091066b0d3387bf4f",
  "vendor/bat-native-usermodel": "https://github.com/brave-intl/bat-native-usermodel.git@c3b6111aa862c5c452c84be8a225d5f1df32b284",
  "vendor/challenge_bypass_ristretto_ffi": "https://github.com/brave-intl/challenge-bypass-ristretto-ffi.git@8b181292ec5fe2ace58cbedfb4d1eab6ec734560",
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

use_relative_paths = True

deps = {
  "vendor/ad-block": "https://github.com/brave/ad-block.git@11ab48c4b2bd5355e789d4c87005ab2c14eecc09",
  "vendor/autoplay-whitelist": "https://github.com/brave/autoplay-whitelist.git@a85d71af4e416cf8188dc297320b0d777aafa315",
  "vendor/extension-whitelist": "https://github.com/brave/extension-whitelist.git@463e5e4e06e0ca84927176e8c72f6076ae9b6829",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@29b1f86b11a8c7438fd7d57b446a77a84946712a",
  "vendor/hashset-cpp": "https://github.com/brave/hashset-cpp.git@4b55fe39bb25bb0d8b11a43d547d75f00c6c46fb",
  "vendor/bloom-filter-cpp": "https://github.com/brave/bloom-filter-cpp.git@9be5c63b14e094156e00c8b28f205e7794f0b92c",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/techtonik/python-patch@2148d5408fafd5c05ac6854dd7deb4e8a4ca4a49",
  "vendor/omaha":  "https://github.com/brave/omaha.git@22dec7e124881ba2c7e8f331d18d9c4dd40ed207",
  "vendor/sparkle": "https://github.com/brave/Sparkle.git@c0759cce415d7c0feae45005c8a013b1898711f0",
  "vendor/bat-native-rapidjson": "https://github.com/brave-intl/bat-native-rapidjson.git@86aafe2ef89835ae71c9ed7c2527e3bb3000930e",
  "vendor/bip39wally-core-native": "https://github.com/brave-intl/bip39wally-core-native.git@9b119931c702d55be994117eb505d56310720b1d",
  "vendor/bat-native-anonize": "https://github.com/brave-intl/bat-native-anonize.git@e3742ba3e8942eea9e4755d91532491871bd3116",
  "vendor/bat-native-tweetnacl": "https://github.com/brave-intl/bat-native-tweetnacl.git@1b4362968c8f22720bfb75af6f506d4ecc0f3116",
  "components/brave_sync/extension/brave-sync": "https://github.com/brave/sync.git@951e07120166ab1d7b1857424676c6ae1b27f49f",
  "components/brave_sync/extension/brave-crypto": "https://github.com/brave/crypto@54f35a7bd6e297620c6add188f1b31af3d0ec561",
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

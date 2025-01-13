use_relative_paths = True

vars = {
  'download_prebuilt_sparkle': True,
  'checkout_dmg_tool': False,
}

deps = {
  "vendor/python-patch": "https://github.com/brave/python-patch@d8880110be6554686bc08261766538c2926d4e82",
  "vendor/omaha": {
    "url": "https://github.com/brave/omaha.git@32383a4dc9c50a88e42be0e03e5b2f2ba7ad058b",
    "condition": "checkout_win",
  },
  "vendor/sparkle": {
    "url": "https://github.com/brave/Sparkle.git@8721f93f694244f9ff41fe975a92617ac5f63f9a",
    "condition": "checkout_mac",
  },
  "vendor/bat-native-tweetnacl": "https://github.com/brave-intl/bat-native-tweetnacl.git@800f9d40b7409239ff192e0be634764e747c7a75",
  "vendor/gn-project-generators": "https://github.com/brave/gn-project-generators.git@b76e14b162aa0ce40f11920ec94bfc12da29e5d0",
  "vendor/web-discovery-project": "https://github.com/brave/web-discovery-project@7159cb52cf674da3aed53ad0ab757513703a06eb",
  "third_party/bip39wally-core-native": "https://github.com/brave-intl/bat-native-bip39wally-core.git@0d3a8713a2b388d2156fe49a70ef3f7cdb44b190",
  "third_party/ethash/src": "https://github.com/chfast/ethash.git@e4a15c3d76dc09392c7efd3e30d84ee3b871e9ce",
  "third_party/bitcoin-core/src": "https://github.com/bitcoin/bitcoin.git@8105bce5b384c72cf08b25b7c5343622754e7337", # v25.0
  "third_party/argon2/src": "https://github.com/P-H-C/phc-winner-argon2.git@62358ba2123abd17fccf2a108a301d4b52c01a7c",
  "third_party/libdmg-hfsplus": {
    "url": "https://github.com/fanquake/libdmg-hfsplus.git@1cc791e4173da9cb0b0cc16c5a1aaa25d5eb5efa",
    "condition": 'checkout_mac and host_os != "mac" and checkout_dmg_tool',
  },
  "third_party/rapidjson/src": "https://github.com/Tencent/rapidjson.git@06d58b9e848c650114556a23294d0b6440078c61",
  "third_party/reclient_configs/src": "https://github.com/EngFlow/reclient-configs.git@21c8fe69ff771956c179847b8c1d9fd216181967",
  'third_party/android_deps/libs/com_google_android_play_core': {
      'packages': [
          {
              'package': 'chromium/third_party/android_deps/libs/com_google_android_play_core',
              'version': 'version:2@1.10.3.cr1',
          },
      ],
      'condition': 'checkout_android',
      'dep_type': 'cipd',
  },
  "third_party/playlist_component/src": "https://github.com/brave/playlist-component.git@673d40f017a1559bb685a15cf608ad1d4a94f8fb",
  "third_party/rust/futures_retry/v0_5/crate": "https://github.com/brave-intl/futures-retry.git@2aaaafbc3d394661534d4dbd14159d164243c20e",
  "third_party/macholib": {
    "url": "https://github.com/ronaldoussoren/macholib.git@36a6777ccd0891c5d1b44ba885573d7c90740015",
    "condition": "checkout_mac",
  },
  "components/brave_wallet/browser/zcash/rust/librustzcash/src": "https://github.com/brave/librustzcash.git@127aacc83dc9ed12fc38c3c7f5b52f7f51011e4d", # v2
}

recursedeps = [
  'vendor/omaha'
]

hooks = [
  {
    'name': 'bootstrap',
    'pattern': '.',
    'action': ['vpython3', 'script/bootstrap.py'],
  },
  {
    'name': 'bootstrap_ios',
    'pattern': '.',
    'condition': 'checkout_ios and host_os == "mac"',
    'action': ['python3', 'script/ios_bootstrap.py']
  },
  {
    # Download hermetic xcode for goma
    'name': 'download_hermetic_xcode',
    'pattern': '.',
    'condition': 'checkout_mac or checkout_ios',
    'action': ['vpython3', 'build/mac/download_hermetic_xcode.py'],
  },
  {
    'name': 'configure_reclient',
    'pattern': '.',
    'action': ['python3', 'third_party/reclient_configs/src/configure_reclient.py',
               '--src_dir=..',
               '--exec_root=../..',
               '--custom_py=third_party/reclient_configs/brave_custom/brave_custom.py'],
  },
  {
    'name': 'download_sparkle',
    'pattern': '.',
    'condition': 'checkout_mac and download_prebuilt_sparkle',
    'action': ['vpython3', 'build/download_dep.py',
               'sparkle/sparkle-1.24.3.tar.gz',
               '//build/mac_files/sparkle_binaries'],
  },
  {
    'name': 'download_omaha4',
    'pattern': '.',
    'condition': 'checkout_mac',
    'action': ['vpython3', 'build/download_dep.py',
               'omaha4/BraveUpdater-132.1.75.66.zip',
               '//build/mac_files/omaha4'],
  },
  {
    'name': 'update_pip',
    'pattern': '.',
    # Required for download_cryptography below. Specifically, newer versions of
    # pip are required for obtaining binary wheels on Arm64 macOS.
    'action': ['python3', '-m', 'pip', '-q', '--disable-pip-version-check', 'install', '-U', '--no-warn-script-location', 'pip'],
  },
  {
    'name': 'download_cryptography',
    'pattern': '.',
    # We don't include cryptography as a DEP because building it from source is
    # difficult. We pin to a version >=37.0.2 and <38.0.0 to avoid an
    # incompatibility with our pyOpenSSL version on Android. See:
    # https://github.com/pyca/cryptography/issues/7126.
    # We use python3 instead of vpython3 for two reasons: First, our GN actions
    # are run with python3, so this environment mirrors the one in which
    # cryptography will be used. Second, we cannot update pip in vpython3 on at
    # least macOS due to permission issues.
    'action': ['python3', '-m', 'pip', '-q', '--disable-pip-version-check', 'install', '-U', '-t', 'third_party/cryptography', '--only-binary', 'cryptography', 'cryptography==37.0.4'],
  },
  {
    'name': 'wireguard_nt',
    'pattern': '.',
    'condition': 'checkout_win',
    'action': ['vpython3', 'build/download_dep.py',
               'brave-vpn-wireguard-dlls/brave-vpn-wireguard-nt-dlls-0.10.1.zip',
               '//brave/third_party/brave-vpn-wireguard-nt-dlls'],
  },
  {
    'name': 'wireguard_tunnel',
    'pattern': '.',
    'condition': 'checkout_win',
    'action': ['vpython3', 'build/download_dep.py',
               'brave-vpn-wireguard-dlls/brave-vpn-wireguard-tunnel-dlls-v0.5.3.zip',
               '//brave/third_party/brave-vpn-wireguard-tunnel-dlls'],
  },
  {
    # Install Web Discovery Project dependencies for Windows, Linux, and macOS
    'name': 'web_discovery_project_npm_deps',
    'pattern': '.',
    'condition': 'checkout_linux or checkout_mac or checkout_win',
    'action': ['vpython3', 'script/web_discovery_project.py', '--install'],
  },
  {
    'name': 'generate_licenses',
    'pattern': '.',
    'action': ['vpython3', 'script/generate_licenses.py'],
  },
  {
    # Overwrite Chromium's LASTCHANGE using the latest Brave version commit.
    'name': 'brave_lastchange',
    'pattern': '.',
    'action': ['python3', '../build/util/lastchange.py',
               '--output', '../build/util/LASTCHANGE',
               '--source-dir', '.',
               '--filter', '^[0-9]\{{1,\}}\.[0-9]\{{1,\}}\.[0-9]\{{1,\}}$'],
  },
  {
    # Downloads & overwrites Chromium's swift-format dep on macOS only
    'name': 'download_swift_format',
    'pattern': '.',
    'condition': 'host_os == "mac"',
    'action': ['python3', 'build/apple/download_swift_format.py', '510.1.0', '0ddbb486640cde862fa311dc0f7387e6c5171bdcc0ee0c89bc9a1f8a75e8bfaf']
  },
  {
    # Generate .clang-format.
    'name': 'generate_clang_format',
    'pattern': '.',
    'action': ['vpython3', 'build/util/generate_clang_format.py', '../.clang-format', '.clang-format']
  },
  {
    'name': 'update_midl_files',
    'pattern': '.',
    'condition': 'checkout_win',
    'action': ['python3', 'build/util/update_midl_files.py']
  },
  {
    'name': 'build_libdmg_hfsplus',
    'pattern': '.',
    "condition": 'checkout_mac and host_os != "mac" and checkout_dmg_tool',
    'action': ['build/mac/cross-compile/build-libdmg-hfsplus.py', 'third_party/libdmg-hfsplus']
  },
]

include_rules = [
  "-chrome",
  "-brave",
  "-third_party/rust",

  # Everybody can use some things.
  "+brave/base",
  "+brave/brave_domains",
]

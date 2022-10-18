# Since recent changes upstream, Chromium is now using IDs up to ~40000
# (see end of //out/Component/gen/tools/gritsettings/default_resource_ids),
# so let's leave some padding after that and start assigning them on 45000.
{
  "SRCDIR": "../..",
  "brave/common/extensions/api/brave_api_resources.grd": {
    "includes": [45050],
  },
  "brave/components/resources/brave_components_resources.grd": {
    "includes": [45100],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_adblock/brave_adblock.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [45200],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_new_tab/brave_new_tab.grd": {
    "META": {"sizes": {"includes": [350]}},
    "includes": [45250],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_welcome/brave_welcome.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [45600],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/browser/resources/settings/brave_settings_resources.grd": {
    "META": {"sizes": {"includes": [400]}},
    "includes": [45800],
  },
  "brave/app/brave_generated_resources.grd": {
    "includes": [46200],
    "messages": [46700],
  },
  "brave/app/theme/brave_theme_resources.grd": {
    "structures": [47200],
  },
  "brave/app/theme/brave_unscaled_resources.grd": {
    "includes": [47700],
  },
  "brave/components/brave_sync/resources.grd": {
    "includes": [48200]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_sync/brave_sync.grd": {
    "META": {"sizes": {"includes": [500]}},
    "includes": [48300],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_webtorrent/brave_webtorrent.grd": {
    "META": {"sizes": {"includes": [500]}},
    "includes": [48800],
  },
  "brave/components/brave_rewards/resources/extension/extension_static_resources.grd": {
    "includes": [49300]
  },
  "brave/components/brave_webtorrent/resources.grd": {
    "includes": [49400]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_extension_panel/brave_rewards_extension_panel.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [49500],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_panel/brave_rewards_panel.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [49600],
  },
  "brave/components/brave_rewards/resources/brave_rewards_static_resources.grd": {
    "includes": [49700]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_page/brave_rewards_page.grd": {
    "META": {"sizes": {"includes": [500]}},
    "includes": [49900],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_internals/brave_rewards_internals.grd": {
    "META": {"sizes": {"includes": [300]}},
    "includes": [50400],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_tip/brave_rewards_tip.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [50700],
  },
  "brave/components/resources/brave_components_strings.grd": {
    "messages": [50900]
  },
  "brave/vendor/bat-native-ads/data/resources/bat_ads_resources.grd": {
    "includes": [51000]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_page/brave_wallet_page.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [54600],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ethereum_remote_client_page/ethereum_remote_client_page.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [54700],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_panel/brave_wallet_panel.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [54800],
  },
  "brave/components/brave_extension/extension/resources.grd": {
    "includes": [55000],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_extension/brave_extension.grd": {
    "META": {"sizes": {"includes": [500]}},
    "includes": [55500],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-webcompat_reporter/webcompat_reporter.grd": {
    "META": {"sizes": {"includes": [500]}},
    "includes": [56000],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ipfs/ipfs.grd": {
    "META": {"sizes": {"includes": [500]}},
    "includes": [56500],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-cosmetic_filters/cosmetic_filters.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [57000]
  },
  "brave/components/tor/resources/tor_static_resources.grd": {
    "includes": [57250]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-tor_internals/tor_internals.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [57500],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_script/brave_wallet_script.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [57750]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_vpn_panel/brave_vpn_panel.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [58000]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_shields_panel/brave_shields_panel.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [58050]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-trezor_bridge/trezor_bridge.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [58250]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/browser/resources/sidebar/sidebar_resources.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [58500]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/browser/resources/federated_internals/federated_internals_resources.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [58750]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-market_display/market_display.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [59000]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_private_new_tab/brave_private_new_tab.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [59020]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-playlist/playlist.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [59270]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ledger_bridge/ledger_bridge.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [59520]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-nft_display/nft_display.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [59770]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-cookie_list_opt_in/cookie_list_opt_in.grd": {
    "META": {"sizes": {"includes": [30]}},
    "includes": [59800]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_speedreader_panel/brave_speedreader_panel.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [59820]
  }
}

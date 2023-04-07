# Since recent changes upstream, Chromium is now using IDs up to ~45090
# (see end of //out/Component/gen/tools/gritsettings/default_resource_ids),
# so let's leave some padding after that and start assigning them on 45250.
{
  "SRCDIR": "../..",
  "brave/common/extensions/api/brave_api_resources.grd": {
    "includes": [45250],
  },
  "brave/components/resources/brave_components_resources.grd": {
    "includes": [45300],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_adblock/brave_adblock.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [45400],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_new_tab/brave_new_tab.grd": {
    "META": {"sizes": {"includes": [350]}},
    "includes": [45450],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_welcome/brave_welcome.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [45800],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/browser/resources/settings/brave_settings_resources.grd": {
    "META": {"sizes": {"includes": [400]}},
    "includes": [46000],
  },
  "brave/app/brave_generated_resources.grd": {
    "includes": [46400],
    "messages": [46900],
  },
  "brave/app/theme/brave_theme_resources.grd": {
    "structures": [47400],
  },
  "brave/app/theme/brave_unscaled_resources.grd": {
    "includes": [47900],
  },
  "brave/components/brave_sync/resources.grd": {
    "includes": [48400]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_sync/brave_sync.grd": {
    "META": {"sizes": {"includes": [500]}},
    "includes": [48500],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_webtorrent/brave_webtorrent.grd": {
    "META": {"sizes": {"includes": [500]}},
    "includes": [49000],
  },
  "brave/components/brave_rewards/resources/extension/extension_static_resources.grd": {
    "includes": [49500]
  },
  "brave/components/brave_webtorrent/resources.grd": {
    "includes": [49600]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_extension_panel/brave_rewards_extension_panel.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [49700],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_panel/brave_rewards_panel.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [49800],
  },
  "brave/components/brave_rewards/resources/brave_rewards_static_resources.grd": {
    "includes": [49900]
  },
  "brave/components/brave_vpn/resources/panel/static_resources.grd": {
    "includes": [50000]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_page/brave_rewards_page.grd": {
    "META": {"sizes": {"includes": [500]}},
    "includes": [50100],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_internals/brave_rewards_internals.grd": {
    "META": {"sizes": {"includes": [300]}},
    "includes": [50600],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_tip/brave_rewards_tip.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [50900],
  },
  "brave/components/resources/brave_components_strings.grd": {
    "messages": [51100]
  },
  "brave/components/brave_ads/resources/bat_ads_resources.grd": {
    "includes": [51200]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_page/brave_wallet_page.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [54800],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ethereum_remote_client_page/ethereum_remote_client_page.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [54900],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_panel/brave_wallet_panel.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [55000],
  },
  "brave/components/brave_extension/extension/resources.grd": {
    "includes": [55200],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_extension/brave_extension.grd": {
    "META": {"sizes": {"includes": [500]}},
    "includes": [55700],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-webcompat_reporter/webcompat_reporter.grd": {
    "META": {"sizes": {"includes": [500]}},
    "includes": [56200],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ipfs/ipfs.grd": {
    "META": {"sizes": {"includes": [500]}},
    "includes": [56700],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-cosmetic_filters/cosmetic_filters.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [57200]
  },
  "brave/components/tor/resources/tor_static_resources.grd": {
    "includes": [57450]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-tor_internals/tor_internals.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [57700],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_script/brave_wallet_script.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [57950]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_vpn_panel/brave_vpn_panel.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [58200]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_shields_panel/brave_shields_panel.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [58250]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-trezor_bridge/trezor_bridge.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [58450]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/browser/resources/sidebar/sidebar_resources.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [58700]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/browser/resources/federated_internals/federated_internals_resources.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [58950]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-market_display/market_display.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [59200]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_private_new_tab/brave_private_new_tab.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [59220]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-playlist/playlist.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [59470]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ledger_bridge/ledger_bridge.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [59720]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-nft_display/nft_display.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [59970]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-cookie_list_opt_in/cookie_list_opt_in.grd": {
    "META": {"sizes": {"includes": [30]}},
    "includes": [60000]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_speedreader_panel/brave_speedreader_panel.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [60020]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_adblock_internals/brave_adblock_internals.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [60040],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-commands/commands.grd": {
    "META": {"sizes": {"includes": [250]}},
    "includes": [60290]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_swap_page/brave_wallet_swap_page.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [60300],
  }
}

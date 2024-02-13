# Since recent changes upstream, Chromium is now using IDs up to ~46310
# (see end of //out/Component/gen/tools/gritsettings/default_resource_ids),
# so let's leave some padding after that and start assigning them on 47500.
{
  "SRCDIR": "../..",
  "brave/common/extensions/api/brave_api_resources.grd": {
    "includes": [55400],
  },
  "brave/components/resources/brave_components_resources.grd": {
    "includes": [55410],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_adblock/brave_adblock.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [55600],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_new_tab/brave_new_tab.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [55620],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_welcome/brave_welcome.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [55670],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/browser/resources/settings/brave_settings_resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [55690],
  },
  "brave/app/brave_generated_resources.grd": {
    "includes": [55700],
    "messages": [55750],
  },
  "brave/app/theme/brave_theme_resources.grd": {
    "structures": [56700],
  },
  "brave/app/theme/brave_unscaled_resources.grd": {
    "includes": [56800],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_webtorrent/brave_webtorrent.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [56810],
  },
  "brave/components/brave_webtorrent/resources.grd": {
    "includes": [56820],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_panel/brave_rewards_panel.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [56830],
  },
  "brave/components/brave_rewards/resources/brave_rewards_static_resources.grd": {
    "includes": [56850],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_page/brave_rewards_page.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [56860],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_internals/brave_rewards_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [56880],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_tip/brave_rewards_tip.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [56890],
  },
  "brave/components/resources/brave_components_strings.grd": {
    "messages": [56950],
  },
  "brave/components/brave_ads/resources/bat_ads_resources.grd": {
    "includes": [59450]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_page/brave_wallet_page.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [59460],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ethereum_remote_client_page/ethereum_remote_client_page.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [59660],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_panel/brave_wallet_panel.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [59670],
  },
  "brave/components/brave_extension/extension/resources.grd": {
    "includes": [59950],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_extension/brave_extension.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [59970],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-webcompat_reporter/webcompat_reporter.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [59980],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ipfs/ipfs.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [59990],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-cosmetic_filters/cosmetic_filters.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60000],
  },
  "brave/components/tor/resources/tor_static_resources.grd": {
    "includes": [60010],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-tor_internals/tor_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60020],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_script/brave_wallet_script.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60030],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_vpn_panel/brave_vpn_panel.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60040],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_shields_panel/brave_shields_panel.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60050],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-trezor_bridge/trezor_bridge.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60060],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-market_display/market_display.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [60070],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_private_new_tab/brave_private_new_tab.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60170],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-playlist/playlist.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60180],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ledger_bridge/ledger_bridge.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60190],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-nft_display/nft_display.grd": {
    "META": {"sizes": {"includes": [90]}},
    "includes": [60200],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-cookie_list_opt_in/cookie_list_opt_in.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60290],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_speedreader_toolbar/brave_speedreader_toolbar.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60300],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_adblock_internals/brave_adblock_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60310],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-commands/commands.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60320],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_swap_page/brave_wallet_swap_page.grd": {
    "META": {"sizes": {"includes": [150]}},
    "includes": [60330],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_send_page/brave_wallet_send_page.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [60470],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_deposit_page/brave_wallet_deposit_page.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [60570],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_fund_wallet_page/brave_wallet_fund_wallet_page.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [60670],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-tip_panel/tip_panel.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [60770]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ai_chat_ui/ai_chat_ui.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [60970],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-skus_internals/skus_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60980],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_news_internals/brave_news_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60990],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-line_chart_display/line_chart_display.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [61000]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_player/brave_player.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [61010],
  }
}

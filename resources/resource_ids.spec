# Since recent changes upstream, Chromium is now using IDs up to ~46310
# (see end of //out/Component/gen/tools/gritsettings/default_resource_ids),
# so let's leave some padding after that and start assigning them on 47500.
{
  "SRCDIR": "../..",
  "brave/common/extensions/api/brave_api_resources.grd": {
    "includes": [58700],
  },
  "brave/components/resources/brave_components_resources.grd": {
    "includes": [58710],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_adblock/brave_adblock.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [58900],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_new_tab/brave_new_tab.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [58910],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_welcome/brave_welcome.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [58960],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/browser/resources/settings/brave_settings_resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [58980],
  },
  "brave/app/brave_generated_resources.grd": {
    "includes": [59000],
    "messages": [59050],
  },
  "brave/app/theme/brave_theme_resources.grd": {
    "structures": [60000],
  },
  "brave/app/theme/brave_unscaled_resources.grd": {
    "includes": [60100],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_webtorrent/brave_webtorrent.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60110],
  },
  "brave/components/brave_webtorrent/resources.grd": {
    "includes": [60120],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_panel/brave_rewards_panel.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [60130],
  },
  "brave/components/brave_rewards/resources/brave_rewards_static_resources.grd": {
    "includes": [60150],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_page/brave_rewards_page.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [60160],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_internals/brave_rewards_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [60180],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_rewards_tip/brave_rewards_tip.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [60190],
  },
  "brave/components/resources/brave_components_strings.grd": {
    "messages": [60210],
  },
  "brave/components/brave_ads/resources/bat_ads_resources.grd": {
    "includes": [62750]
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_page/brave_wallet_page.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [62760],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ethereum_remote_client_page/ethereum_remote_client_page.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [62960],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_panel/brave_wallet_panel.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [62970],
  },
  "brave/components/brave_extension/extension/resources.grd": {
    "includes": [63170],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_extension/brave_extension.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [63270],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-webcompat_reporter/webcompat_reporter.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [63280],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-cosmetic_filters/cosmetic_filters.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [63290],
  },
  "brave/components/tor/resources/tor_static_resources.grd": {
    "includes": [63300],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-tor_internals/tor_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [63310],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_script/brave_wallet_script.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [63320],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_vpn_panel/brave_vpn_panel.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [63330],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_shields_panel/brave_shields_panel.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [63340],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-trezor_bridge/trezor_bridge.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [63350],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-market_display/market_display.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [63360],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_private_new_tab/brave_private_new_tab.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [63460],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-playlist/playlist.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [63470],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ledger_bridge/ledger_bridge.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [63480],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-nft_display/nft_display.grd": {
    "META": {"sizes": {"includes": [90]}},
    "includes": [63490],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-cookie_list_opt_in/cookie_list_opt_in.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [63580],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_speedreader_toolbar/brave_speedreader_toolbar.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [63590],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_adblock_internals/brave_adblock_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [63600],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-commands/commands.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [63610],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_swap_page/brave_wallet_swap_page.grd": {
    "META": {"sizes": {"includes": [150]}},
    "includes": [63620],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_send_page/brave_wallet_send_page.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [63770],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_deposit_page/brave_wallet_deposit_page.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [63870],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_fund_wallet_page/brave_wallet_fund_wallet_page.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [63970],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-tip_panel/tip_panel.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [64070]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ai_chat_ui/ai_chat_ui.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [64090],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-skus_internals/skus_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [64290],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_news_internals/brave_news_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [64300],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-line_chart_display/line_chart_display.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [64310]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ai_rewriter_ui/ai_rewriter_ui.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [64320],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-rewards_page/rewards_page.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [64330],
  },
}

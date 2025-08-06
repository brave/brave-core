# Since recent changes upstream, Chromium is now using IDs up to ~59570 (see end
# of //out/Component/gen/tools/gritsettings/default_resource_ids). We previously
# added our strings after all of the upstream strings, but we are now too close
# to the maximum id (65536) to continue doing that. Instead, we now overlap with
# a section of upstream strings related to ChromeOS/Ash which are unused by us.
# This range runs from 29300-36930, so it should provide a decent amount of
# room for future growth.
{
  "SRCDIR": "../..",
  "brave/common/extensions/api/brave_api_resources.grd": {
    "includes": [29580],
  },
  "brave/components/resources/brave_components_resources.grd": {
    "includes": [29600],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_adblock/brave_adblock.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [29620],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_new_tab/brave_new_tab.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [29640],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_welcome/brave_welcome.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [29660],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/browser/resources/settings/brave_settings_resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [29680],
  },
  "brave/app/brave_generated_resources.grd": {
    "includes": [29700],
    "messages": [29720],
  },
  "brave/app/theme/brave_theme_resources.grd": {
    "structures": [29740],
  },
  "brave/app/theme/brave_unscaled_resources.grd": {
    "includes": [29760],
  },
  "brave/components/brave_rewards/resources/brave_rewards_static_resources.grd": {
    "includes": [29780],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-rewards_internals/rewards_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [29800],
  },
  "brave/components/resources/brave_components_strings.grd": {
    "messages": [29820],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_page/brave_wallet_page.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [29840],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_panel/brave_wallet_panel.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [29860],
  },
  "brave/components/brave_extension/extension/resources.grd": {
    "includes": [29880],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_extension/brave_extension.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [29900],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-webcompat_reporter/webcompat_reporter.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [29920],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-cosmetic_filters/cosmetic_filters.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [29940],
  },
  "brave/components/tor/resources/tor_static_resources.grd": {
    "includes": [29960],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-tor_internals/tor_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [29980],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_script/brave_wallet_script.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30000],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_vpn_panel/brave_vpn_panel.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30020],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_shields_panel/brave_shields_panel.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30040],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-trezor_bridge/trezor_bridge.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30060],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-market_display/market_display.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [30080],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_private_new_tab/brave_private_new_tab.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30100],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-playlist/playlist.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30120],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ledger_bridge/ledger_bridge.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30140],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-nft_display/nft_display.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30160],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_speedreader_toolbar/brave_speedreader_toolbar.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30180],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_adblock_internals/brave_adblock_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30200],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_swap_page/brave_wallet_swap_page.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [30220],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_send_page/brave_wallet_send_page.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [30240],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_deposit_page/brave_wallet_deposit_page.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [30260],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_fund_wallet_page/brave_wallet_fund_wallet_page.grd": {
    "META": {"sizes": {"includes": [80]}},
    "includes": [30280],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-tip_panel/tip_panel.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [30300]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ai_chat_ui/ai_chat_ui.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30320],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-skus_internals/skus_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30340],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_news_internals/brave_news_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30360],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-line_chart_display/line_chart_display.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30380]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ai_rewriter_ui/ai_rewriter_ui.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30400],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-rewards_page/rewards_page.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [30420],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-creator_detection/creator_detection.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30440],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ads_internals/ads_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30460],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-custom_site_distiller_scripts/custom_site_distiller_scripts.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [30480],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/browser/resources/brave_education/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30500],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-new_tab_takeover/new_tab_takeover.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [30520],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_new_tab_page_refresh/brave_new_tab_page_refresh.grd": {
    "META": {"sizes": {"includes": [30]}},
    "includes": [30540],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-email_aliases/email_aliases.grd": {
    "META": {"sizes": {"includes": [40]}},
    "includes": [30560],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/components/brave_account/resources/resources.grd": {
    "META": {"sizes": {"includes": [25]}},
    "includes": [30580],
  },
  # WARNING: The upstream ChromeOS/Ash strings currently run through 36930. We
  # must be careful not to exceed that maximum when adding new strings here.
}

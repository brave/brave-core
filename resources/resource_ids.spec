# Since cr144 changes upstream, Chromium is now using almost the entire ID
# range of //out/<BUILD_TYPE>/gen/tools/gritsettings/default_resource_ids). We
# now filter out all chromeos/ash entries by doing additional pre-processing of
# tools/gritsettings/resource_ids.spec via //brave/tools/gritsettings filter.
# Currently, after filtering the upstreams IDs range reaches ~43,500. Leaving
# a gap for upstream to expand, we will be starting our strings from 53,000.
# This would currently put the end of our strings at ~58,000.
{
  "SRCDIR": "../..",
  "brave/common/extensions/api/brave_api_resources.grd": {
    "includes": [53000],
  },
  "brave/components/resources/brave_components_resources.grd": {
    "includes": [53020],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_adblock/brave_adblock.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53040],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_new_tab/brave_new_tab.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [53060],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_welcome/brave_welcome.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [53080],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/browser/resources/settings/brave_settings_resources.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [53100],
  },
  "brave/app/brave_generated_resources.grd": {
    "includes": [53120],
    "messages": [53140],
  },
  "brave/app/theme/brave_theme_resources.grd": {
    "structures": [53160],
  },
  "brave/app/theme/brave_unscaled_resources.grd": {
    "includes": [53180],
  },
  "brave/components/brave_rewards/resources/brave_rewards_static_resources.grd": {
    "includes": [53200],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-rewards_internals/rewards_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53220],
  },
  "brave/components/resources/brave_components_strings.grd": {
    "messages": [53240],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_page/brave_wallet_page.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [53260],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_panel/brave_wallet_panel.grd": {
    "META": {"sizes": {"includes": [200]}},
    "includes": [53280],
  },
  "brave/components/brave_extension/extension/resources.grd": {
    "includes": [53300],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_extension/brave_extension.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53320],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-webcompat_reporter/webcompat_reporter.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53340],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-cosmetic_filters/cosmetic_filters.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53360],
  },
  "brave/components/tor/resources/tor_static_resources.grd": {
    "includes": [53380],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-tor_internals/tor_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53400],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_script/brave_wallet_script.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53420],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_vpn_panel/brave_vpn_panel.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53440],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_shields_panel/brave_shields_panel.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53460],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-trezor_bridge/trezor_bridge.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53480],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-market_display/market_display.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [53500],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_private_new_tab/brave_private_new_tab.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53520],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-playlist/playlist.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53540],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ledger_bridge/ledger_bridge.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53560],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-nft_display/nft_display.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53580],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_speedreader_toolbar/brave_speedreader_toolbar.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53600],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_adblock_internals/brave_adblock_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53620],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_swap_page/brave_wallet_swap_page.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [53640],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_send_page/brave_wallet_send_page.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [53660],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_deposit_page/brave_wallet_deposit_page.grd": {
    "META": {"sizes": {"includes": [100]}},
    "includes": [53680],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_wallet_fund_wallet_page/brave_wallet_fund_wallet_page.grd": {
    "META": {"sizes": {"includes": [80]}},
    "includes": [53700],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-tip_panel/tip_panel.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [53720]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ai_chat_ui/ai_chat_ui.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [53740],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-skus_internals/skus_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53760],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_news_internals/brave_news_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53780],
  },
  # This file is generated during the build.
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-line_chart_display/line_chart_display.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53800]
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ai_rewriter_ui/ai_rewriter_ui.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53820],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-rewards_page/rewards_page.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [53840],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-creator_detection/creator_detection.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53860],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ads_internals/ads_internals.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53880],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-custom_site_distiller_scripts/custom_site_distiller_scripts.grd": {
    "META": {"sizes": {"includes": [50]}},
    "includes": [53900],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/browser/resources/brave_education/resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53920],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-new_tab_takeover/new_tab_takeover.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [53940],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_new_tab_page_refresh/brave_new_tab_page_refresh.grd": {
    "META": {"sizes": {"includes": [30]}},
    "includes": [53960],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-email_aliases/email_aliases.grd": {
    "META": {"sizes": {"includes": [40]}},
    "includes": [53980],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/components/brave_account/resources/resources.grd": {
    "META": {"sizes": {"includes": [35]}},
    "includes": [54000],
  },
  "brave/ios/web/test/test_resources.grd": {
    "META": {"sizes": {"includes": [10]}},
    "includes": [54020],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/browser/resources/ai_chat_agent_new_tab_page/ai_chat_agent_new_tab_page_static_resources.grd": {
    "META": {"sizes": {"includes": [1]}},
    "includes": [54030],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-candle_embedding_gemma/candle_embedding_gemma.grd": {
    "META": {"sizes": {"includes": [2]}},
    "includes": [54031],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-ai_chat_agent_new_tab_page/ai_chat_agent_new_tab_page.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [54040],
  },
  "brave/browser/resources/bookmark_icon/bookmark_icon_resources.grd": {
    "structures": [54060],
  },
  "<(SHARED_INTERMEDIATE_DIR)/brave/web-ui-brave_welcome_page/brave_welcome_page.grd": {
    "META": {"sizes": {"includes": [20]}},
    "includes": [54080],
  },
  # WARNING: The IDs range is 2^16-1. Check
  # out/<BUILD_TYPE>/gen/brave/resources/brave_resource_ids for how much the
  # ids got expanded for the build.
}

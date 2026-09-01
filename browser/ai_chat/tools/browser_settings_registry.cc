// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/browser_settings_registry.h"

#include <algorithm>
#include <string>

#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace ai_chat::browser_settings {

namespace {

// Kept sorted so that lookups can binary search and so that duplicates are
// obvious in review. A unit test enforces both, and also enforces that every
// entry is a registered scalar profile preference.
constexpr std::string_view kAllowedPrefs[] = {
    "accessibility.captions.background_color",
    "accessibility.captions.background_opacity",
    "accessibility.captions.live_caption_enabled",
    "accessibility.captions.live_caption_language",
    "accessibility.captions.live_caption_mask_offensive_words",
    "accessibility.captions.live_translate_enabled",
    "accessibility.captions.live_translate_target_language",
    "accessibility.captions.text_color",
    "accessibility.captions.text_font",
    "accessibility.captions.text_opacity",
    "accessibility.captions.text_shadow",
    "accessibility.captions.text_size",
    "alternate_error_pages.enabled",
    "auto_pin_new_tab_groups",
    "autofill.autofill_ai.identity_entities_enabled",
    "autofill.autofill_ai.shopping_entities_enabled",
    "autofill.autofill_ai.travel_entities_enabled",
    "autofill.bnpl_enabled",
    "autofill.credit_card_enabled",
    "autofill.email_verification_enabled",
    "autofill.payment_card_benefits",
    "autofill.payment_cvc_storage",
    "autofill.profile_enabled",
    "bookmark_bar.show_on_all_tabs",
    "bookmark_bar.show_tab_groups",
    "bookmark_bar.visibility_state",
    "brave.ad_block.developer_mode",
    "brave.ai_chat.autocomplete_provider_enabled",
    "brave.ai_chat.context_menu_enabled",
    "brave.ai_chat.ollama_fetch_enabled",
    "brave.ai_chat.show_toolbar_button",
    "brave.ai_chat.storage_enabled",
    "brave.ai_chat.tab_organization_enabled",
    "brave.ai_chat.user_customization_enabled",
    "brave.ai_chat.user_memory_enabled",
    "brave.always_show_bookmark_bar_on_ntp",
    "brave.ask_widevine_install",
    "brave.autocomplete_enabled",
    "brave.autofill_private_windows",
    "brave.de_amp.enabled",
    "brave.debounce.enabled",
    "brave.email_aliases.new_alias_autofill_suggestion_enabled",
    "brave.enable_closing_last_tab",
    "brave.enable_media_router_on_restart",
    "brave.enable_window_closing_confirm",
    "brave.gcm.channel_status",
    "brave.google_login_default",
    "brave.history.retention_days",
    "brave.location_bar_is_wide",
    "brave.mru_cycling_enabled",
    "brave.new_tab_page.show_background_image",
    "brave.new_tab_page.show_branded_background_image",
    "brave.new_tab_page.show_clock",
    "brave.new_tab_page.show_rewards",
    "brave.new_tab_page.show_stats",
    "brave.new_tab_page.show_together",
    "brave.new_tab_page.shows_options",
    "brave.new_tab_page.sponsored_images.survey_panelist",
    "brave.omnibox.bookmark_suggestions_enabled",
    "brave.omnibox.commander_suggestions_enabled",
    "brave.omnibox.history_suggestions_enabled",
    "brave.playlist.cache",
    "brave.playlist.enabled",
    "brave.psst.settings.enable_psst",
    "brave.reduce_language",
    "brave.request_otr.request_otr_action_option",
    "brave.rewards.enabled",
    "brave.rewards.show_brave_rewards_button_in_location_bar",
    "brave.shields.advanced_view_enabled",
    "brave.shields.stats_badge_visible",
    "brave.show_bookmarks_button",
    "brave.show_fullscreen_reminder",
    "brave.show_side_panel_button",
    "brave.sidebar.sidebar_show_option",
    "brave.speedreader.enabled_for_all_sites",
    "brave.speedreader.feature_enabled",
    "brave.subtle_app_menu_logo",
    "brave.tabs.always_hide_tab_close_button",
    "brave.tabs.always_use_mini_accent_icon",
    "brave.tabs.hover_mode",
    "brave.tabs.middle_click_close_tab_enabled",
    "brave.tabs.min_width_mode",
    "brave.tabs.mute_indicator_not_clickable",
    "brave.tabs.scrollable_horizontal_tab_strip",
    "brave.tabs.shared_pinned_tab",
    "brave.tabs.show_horizontal_tab_scroll_buttons",
    "brave.tabs.tree_tabs_enabled",
    "brave.tabs.vertical_tabs_enabled",
    "brave.tabs.vertical_tabs_expanded_state_per_window",
    "brave.tabs.vertical_tabs_floating_enabled",
    "brave.tabs.vertical_tabs_hide_completely_when_collapsed",
    "brave.tabs.vertical_tabs_on_right",
    "brave.tabs.vertical_tabs_show_scrollbar",
    "brave.tabs.vertical_tabs_show_title_on_window",
    "brave.tabs.vertical_tabs_show_toggle_button",
    "brave.today.should_show_toolbar_button",
    "brave.top_site_suggestions_enabled",
    "brave.wallet.auto_lock_minutes",
    "brave.wallet.default_base_cryptocurrency",
    "brave.wallet.default_base_currency",
    "brave.wallet.default_cardano_wallet",
    "brave.wallet.default_solana_wallet",
    "brave.wallet.default_wallet2",
    "brave.wallet.nft_discovery_enabled",
    "brave.wallet.private_windows_enabled",
    "brave.wallet.show_wallet_icon_on_toolbar",
    "brave.wallet.transaction_simulation_opt_in_status",
    "brave.wayback_machine_enabled",
    "brave.web_discovery_enabled",
    "brave.web_view_rounded_corners",
    "brave.webcompat.report.enable_save_contact_info",
    "browser.clear_data.brave_leo",
    "browser.clear_data.brave_leo_on_exit",
    "browser.clear_data.browsing_history",
    "browser.clear_data.browsing_history_on_exit",
    "browser.clear_data.cache",
    "browser.clear_data.cache_on_exit",
    "browser.clear_data.cookies",
    "browser.clear_data.cookies_on_exit",
    "browser.clear_data.download_history",
    "browser.clear_data.download_history_on_exit",
    "browser.clear_data.form_data",
    "browser.clear_data.form_data_on_exit",
    "browser.clear_data.hosted_apps_data",
    "browser.clear_data.hosted_apps_data_on_exit",
    "browser.clear_data.passwords",
    "browser.clear_data.passwords_on_exit",
    "browser.clear_data.site_settings",
    "browser.clear_data.site_settings_on_exit",
    "browser.clear_data.time_period",
    "browser.ctrl_tab_mru",
    "browser.custom_chrome_frame",
    "browser.enable_spellchecking",
    "browser.pin_contextual_task_button",
    "browser.pin_split_tab_button",
    "browser.show_forward_button",
    "browser.show_home_button",
    "browser.split_view_drag_and_drop_enabled",
    "cpu_performance_tier_override",
    "credentials_enable_automatic_passkey_upgrades",
    "credentials_enable_autosignin",
    "credentials_enable_service",
    "default_search_provider.enabled",
    "download.prompt_for_download",
    "download_bubble.partial_view_enabled",
    "enable_do_not_track",
    "everything_menu.pinned_to_tabstrip",
    "extensions.theme.system_theme",
    "homepage_is_newtabpage",
    "https_first_mode_bundle_toast_queued",
    "https_only_mode_enabled",
    "import_dialog_autofill_form_data",
    "import_dialog_bookmarks",
    "import_dialog_extensions",
    "import_dialog_history",
    "import_dialog_payments",
    "import_dialog_saved_passwords",
    "import_dialog_search_engine",
    "intl.accept_languages",
    "intl.charset_default",
    "intl.selected_languages",
    "media_router.media_remoting.enabled",
    "net.network_prediction_options",
    "omnibox.keyword_space_triggering_enabled",
    "omnibox.prevent_url_elisions",
    "organizer_panel.pinned_to_tabstrip",
    "password_manager.password_sharing_enabled",
    "payments.can_make_payment_enabled",
    "plugins.always_open_pdf_externally",
    "privacy_guide.viewed",
    "privacy_sandbox.first_party_sets_enabled",
    "privacy_sandbox.m1.ad_measurement_enabled",
    "privacy_sandbox.m1.fledge_enabled",
    "privacy_sandbox.m1.topics_enabled",
    "profile.content_settings.enable_quiet_permission_ui.notifications",
    "profile.cookie_controls_mode",
    "profile.managed.extensions_may_request_permissions",
    "profile.password_dismiss_compromised_alert",
    "profile.password_manager_leak_detection",
    "safebrowsing.bundle",
    "safebrowsing.enabled",
    "safebrowsing.enhanced",
    "safebrowsing.scout_reporting_enabled",
    "safety_hub.unused_site_permissions_revocation.enabled",
    "search.suggest_enabled",
    "session.restore_on_startup",
    "settings.a11y.caretbrowsing.enabled",
    "settings.a11y.enable_accessibility_image_labels",
    "settings.a11y.enable_ax_tree_fixing",
    "settings.a11y.enable_main_node_annotations",
    "settings.a11y.focus_highlight",
    "side_panel.is_right_aligned",
    "signin.allowed_on_next_startup",
    "skills.enabled",
    "spellcheck.use_spelling_service",
    "tab_search.pinned_to_tabstrip",
    "tor.onion_only_in_tor_windows",
    "tracking_protection.block_all_3pc_toggle_enabled",
    "translate.enabled",
    "translate_recent_target",
    "url_keyed_anonymized_data_collection.enabled",
    "vertical_tabs.enabled",
    "vertical_tabs.expand_on_hover",
    "webkit.webprefs.default_fixed_font_size",
    "webkit.webprefs.default_font_size",
    "webkit.webprefs.encrypted_media_enabled",
    "webkit.webprefs.fonts.fixed.Zyyy",
    "webkit.webprefs.fonts.math.Zyyy",
    "webkit.webprefs.fonts.sansserif.Zyyy",
    "webkit.webprefs.fonts.serif.Zyyy",
    "webkit.webprefs.fonts.standard.Zyyy",
    "webkit.webprefs.minimum_font_size",
    "webrtc.ip_handling_policy",
};

// Below this, a match is more likely to be noise than a real answer.
constexpr double kScoreThreshold = 0.3;

// Words that either appear in most paths or carry no signal about which
// setting is meant. "brave" and "browser" matter most here: without them,
// every "brave.*" path would match any question mentioning Brave.
constexpr auto kStopWords = base::MakeFixedFlatSet<std::string_view>({
    "a",          "am",      "an",       "and",  "any",   "are",     "brave",
    "browser",    "can",     "did",      "do",   "does",  "enabled", "for",
    "get",        "has",     "have",     "how",  "i",     "in",      "is",
    "it",         "me",      "my",       "of",   "on",    "or",      "pref",
    "preference", "setting", "settings", "tell", "that",  "the",     "to",
    "turned",     "use",     "value",    "what", "whats", "which",   "you",
    "your",
});

std::vector<std::string> Tokenize(std::string_view text) {
  const std::string lower = base::ToLowerASCII(text);
  return base::SplitString(lower, " \t\n\r.,_-/\\()[]{}\"'?!:;+&*",
                           base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
}

// How well a single query token matches a single token from a preference path.
double TokenScore(const std::string& query_token, const std::string& token) {
  if (query_token == token) {
    return 1.0;
  }
  // Prefix matches let "fingerprint" find "fingerprinting", and let a
  // singular/plural mismatch still score.
  if (query_token.size() >= 3 && token.starts_with(query_token)) {
    return 0.8;
  }
  if (token.size() >= 3 && query_token.starts_with(token)) {
    return 0.6;
  }
  if (query_token.size() >= 4 && token.find(query_token) != std::string::npos) {
    return 0.4;
  }
  return 0.0;
}

}  // namespace

base::span<const std::string_view> GetAllowedPrefs() {
  return base::span(kAllowedPrefs);
}

bool IsAllowedPref(std::string_view pref_name) {
  return std::ranges::binary_search(kAllowedPrefs, pref_name);
}

std::vector<SearchMatch> SearchPrefs(std::string_view query,
                                     size_t max_results) {
  std::vector<std::string> query_tokens;
  for (auto& token : Tokenize(query)) {
    if (!kStopWords.contains(token)) {
      query_tokens.push_back(std::move(token));
    }
  }
  // A query of nothing but stop words ("what are my brave settings") carries
  // no signal about which setting is meant. Matching on the filler would
  // return an arbitrary slice of the registry, so return nothing instead and
  // let the assistant say it couldn't find anything.
  if (query_tokens.empty() || max_results == 0) {
    return {};
  }

  std::vector<SearchMatch> matches;
  for (const auto& pref_name : kAllowedPrefs) {
    // An exact path short-circuits scoring; the assistant is asking for a
    // preference it already knows about.
    if (pref_name == query) {
      return {{.pref_name = pref_name, .score = 1.0}};
    }

    const std::vector<std::string> path_tokens = Tokenize(pref_name);
    double total = 0.0;
    for (const auto& query_token : query_tokens) {
      double best = 0.0;
      for (const auto& path_token : path_tokens) {
        best = std::max(best, TokenScore(query_token, path_token));
      }
      total += best;
    }
    const double score = total / query_tokens.size();
    if (score >= kScoreThreshold) {
      matches.push_back({.pref_name = pref_name, .score = score});
    }
  }

  // Stable sort keeps registry (alphabetical) order for equally-scoring
  // entries, which keeps results deterministic.
  std::ranges::stable_sort(matches, std::ranges::greater(),
                           &SearchMatch::score);
  if (matches.size() > max_results) {
    matches.resize(max_results);
  }
  return matches;
}

}  // namespace ai_chat::browser_settings

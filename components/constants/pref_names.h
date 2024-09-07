/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONSTANTS_PREF_NAMES_H_
#define BRAVE_COMPONENTS_CONSTANTS_PREF_NAMES_H_

#include "build/build_config.h"

inline constexpr char kBraveAutofillPrivateWindows[] =
    "brave.autofill_private_windows";
inline constexpr char kManagedBraveShieldsDisabledForUrls[] =
    "brave.managed_shields_disabled";
inline constexpr char kManagedBraveShieldsEnabledForUrls[] =
    "brave.managed_shields_enabled";
inline constexpr char kAdsBlocked[] = "brave.stats.ads_blocked";
// We no longer update this pref, but we keep it around for now because it's
// added to kAdsBlocked when being displayed.
inline constexpr char kTrackersBlocked[] = "brave.stats.trackers_blocked";
inline constexpr char kJavascriptBlocked[] = "brave.stats.javascript_blocked";
inline constexpr char kHttpsUpgrades[] = "brave.stats.https_upgrades";
inline constexpr char kFingerprintingBlocked[] =
    "brave.stats.fingerprinting_blocked";
inline constexpr char kLastCheckYMD[] = "brave.stats.last_check_ymd";
inline constexpr char kLastCheckWOY[] = "brave.stats.last_check_woy";
inline constexpr char kLastCheckMonth[] = "brave.stats.last_check_month";
inline constexpr char kFirstCheckMade[] = "brave.stats.first_check_made";
// Set to true if the user met the threshold requirements and successfully
// sent a ping to the stats-updater server.
inline constexpr char kThresholdCheckMade[] =
    "brave.stats.threshold_check_made";
// Anonymous usage pings enabled
inline constexpr char kStatsReportingEnabled[] =
    "brave.stats.reporting_enabled";
// Serialized query for to send to the stats-updater server. Needs to be saved
// in the case that the user sends the standard usage ping, stops the browser,
// meets the threshold requirements, and then starts the browser before the
// threshold ping was sent.
inline constexpr char kThresholdQuery[] = "brave.stats.threshold_query";
inline constexpr char kWeekOfInstallation[] =
    "brave.stats.week_of_installation";
inline constexpr char kWidevineEnabled[] = "brave.widevine_opted_in";
inline constexpr char kAskEnableWidvine[] = "brave.ask_widevine_install";
inline constexpr char kShowBookmarksButton[] = "brave.show_bookmarks_button";
inline constexpr char kShowSidePanelButton[] = "brave.show_side_panel_button";
inline constexpr char kLocationBarIsWide[] = "brave.location_bar_is_wide";
inline constexpr char kReferralDownloadID[] = "brave.referral.download_id";
inline constexpr char kReferralTimestamp[] = "brave.referral.timestamp";
inline constexpr char kReferralAttemptTimestamp[] =
    "brave.referral.referral_attempt_timestamp";
inline constexpr char kReferralAttemptCount[] =
    "brave.referral.referral_attempt_count";
inline constexpr char kReferralAndroidFirstRunTimestamp[] =
    "brave.referral_android_first_run_timestamp";
inline constexpr char kNoScriptControlType[] = "brave.no_script_default";
inline constexpr char kShieldsAdvancedViewEnabled[] =
    "brave.shields.advanced_view_enabled";
inline constexpr char kShieldsStatsBadgeVisible[] =
    "brave.shields.stats_badge_visible";
inline constexpr char kAdControlType[] = "brave.ad_default";
inline constexpr char kGoogleLoginControlType[] = "brave.google_login_default";
inline constexpr char kWebTorrentEnabled[] = "brave.webtorrent_enabled";
// Deprecated
inline constexpr char kHangoutsEnabled[] = "brave.hangouts_enabled";
inline constexpr char kNewTabPageShowClock[] = "brave.new_tab_page.show_clock";
inline constexpr char kNewTabPageClockFormat[] =
    "brave.new_tab_page.clock_format";
inline constexpr char kNewTabPageShowStats[] = "brave.new_tab_page.show_stats";
inline constexpr char kNewTabPageShowRewards[] =
    "brave.new_tab_page.show_rewards";
inline constexpr char kNewTabPageShowBraveTalk[] =
    "brave.new_tab_page.show_together";
inline constexpr char kNewTabPageShowBraveVPN[] =
    "brave.new_tab_page.show_brave_vpn";
inline constexpr char kNewTabPageHideAllWidgets[] =
    "brave.new_tab_page.hide_all_widgets";
inline constexpr char kNewTabPageShowsOptions[] =
    "brave.new_tab_page.shows_options";
inline constexpr char kBraveNewsIntroDismissed[] =
    "brave.today.intro_dismissed";
inline constexpr char kAlwaysShowBookmarkBarOnNTP[] =
    "brave.always_show_bookmark_bar_on_ntp";
inline constexpr char kBraveDarkMode[] = "brave.dark_mode";
inline constexpr char kBraveShieldsSettingsVersion[] =
    "brave.shields_settings_version";
inline constexpr char kDefaultBrowserPromptEnabled[] =
    "brave.default_browser_prompt_enabled";

inline constexpr char kWebDiscoveryEnabled[] = "brave.web_discovery_enabled";
inline constexpr char kWebDiscoveryCTAState[] = "brave.web_discovery.cta_state";
inline constexpr char kDontAskEnableWebDiscovery[] =
    "brave.dont_ask_enable_web_discovery";
inline constexpr char kBraveSearchVisitCount[] =
    "brave.brave_search_visit_count";

inline constexpr char kBraveGCMChannelStatus[] = "brave.gcm.channel_status";
inline constexpr char kImportDialogExtensions[] = "import_dialog_extensions";
inline constexpr char kImportDialogPayments[] = "import_dialog_payments";
inline constexpr char kMRUCyclingEnabled[] = "brave.mru_cycling_enabled";
inline constexpr char kTabsSearchShow[] = "brave.tabs_search_show";
inline constexpr char kTabMuteIndicatorNotClickable[] =
    "brave.tabs.mute_indicator_not_clickable";
inline constexpr char kDontAskForCrashReporting[] =
    "brave.dont_ask_for_crash_reporting";

// Cast extension requires a browser restart once the setting is toggled.
// kEnableMediaRouterOnRestart is used as a proxy to identify the current
// state of the switch and prefs::kEnableMediaRouter is updated to
// kEnableMediaRouterOnRestart on restart.
inline constexpr char kEnableMediaRouterOnRestart[] =
    "brave.enable_media_router_on_restart";

#if BUILDFLAG(IS_ANDROID)
inline constexpr char kDesktopModeEnabled[] = "brave.desktop_mode_enabled";
inline constexpr char kPlayYTVideoInBrowserEnabled[] =
    "brave.play_yt_video_in_browser_enabled";
inline constexpr char kBackgroundVideoPlaybackEnabled[] =
    "brave.background_video_playback";
inline constexpr char kSafetynetCheckFailed[] = "safetynetcheck.failed";
inline constexpr char kSafetynetStatus[] = "safetynet.status";
#endif

#if !BUILDFLAG(IS_ANDROID)
inline constexpr char kEnableWindowClosingConfirm[] =
    "brave.enable_window_closing_confirm";
inline constexpr char kEnableClosingLastTab[] = "brave.enable_closing_last_tab";
inline constexpr char kShowFullscreenReminder[] =
    "brave.show_fullscreen_reminder";
#endif

inline constexpr char kDefaultBrowserLaunchingCount[] =
    "brave.default_browser.launching_count";

// deprecated
inline constexpr char kOtherBookmarksMigrated[] =
    "brave.other_bookmarks_migrated";

// Obsolete widget removal prefs
#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
inline constexpr char kFTXAccessToken[] = "brave.ftx.access_token";
inline constexpr char kFTXOauthHost[] = "brave.ftx.oauth_host";
inline constexpr char kFTXNewTabPageShowFTX[] = "ftx.new_tab_page.show_ftx";
inline constexpr char kCryptoDotComNewTabPageShowCryptoDotCom[] =
    "crypto_dot_com.new_tab_page.show_crypto_dot_com";
inline constexpr char kCryptoDotComHasBoughtCrypto[] =
    "crypto_dot_com.new_tab_page.has_bought_crypto";
inline constexpr char kCryptoDotComHasInteracted[] =
    "crypto_dot_com.new_tab_page.has_interacted";
inline constexpr char kGeminiAccessToken[] = "brave.gemini.access_token";
inline constexpr char kGeminiRefreshToken[] = "brave.gemini.refresh_token";
inline constexpr char kNewTabPageShowGemini[] =
    "brave.new_tab_page.show_gemini";
#endif

#if !BUILDFLAG(IS_IOS)
inline constexpr char kBinanceAccessToken[] = "brave.binance.access_token";
inline constexpr char kBinanceRefreshToken[] = "brave.binance.refresh_token";
inline constexpr char kNewTabPageShowBinance[] =
    "brave.new_tab_page.show_binance";
inline constexpr char kBraveSuggestedSiteSuggestionsEnabled[] =
    "brave.brave_suggested_site_suggestions_enabled";
#endif

inline constexpr char kBraveCustomSyncUrlEnabled[] =
    "brave.custom_sync_url_enabled";

#endif  // BRAVE_COMPONENTS_CONSTANTS_PREF_NAMES_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/constants/pref_names.h"

const char kManagedBraveShieldsDisabledForUrls[] =
    "brave.managed_shields_disabled";
const char kManagedBraveShieldsEnabledForUrls[] =
    "brave.managed_shields_enabled";
const char kAdsBlocked[] = "brave.stats.ads_blocked";
// We no longer update this pref, but we keep it around for now because it's
// added to kAdsBlocked when being displayed.
const char kTrackersBlocked[] = "brave.stats.trackers_blocked";
const char kJavascriptBlocked[] = "brave.stats.javascript_blocked";
const char kHttpsUpgrades[] = "brave.stats.https_upgrades";
const char kFingerprintingBlocked[] = "brave.stats.fingerprinting_blocked";
const char kLastCheckYMD[] = "brave.stats.last_check_ymd";
const char kLastCheckWOY[] = "brave.stats.last_check_woy";
const char kLastCheckMonth[] = "brave.stats.last_check_month";
const char kFirstCheckMade[] = "brave.stats.first_check_made";
// Set to true if the user met the threshold requirements and successfully
// sent a ping to the stats-updater server.
const char kThresholdCheckMade[] = "brave.stats.threshold_check_made";
// Anonymous usage pings enabled
const char kStatsReportingEnabled[] = "brave.stats.reporting_enabled";
// Serialized query for to send to the stats-updater server. Needs to be saved
// in the case that the user sends the standard usage ping, stops the browser,
// meets the threshold requirements, and then starts the browser before the
// threshold ping was sent.
const char kThresholdQuery[] = "brave.stats.threshold_query";
const char kWeekOfInstallation[] = "brave.stats.week_of_installation";
const char kWidevineOptedIn[] = "brave.widevine_opted_in";
const char kAskWidevineInstall[] = "brave.ask_widevine_install";
const char kShowBookmarksButton[] = "brave.show_bookmarks_button";
const char kShowSidePanelButton[] = "brave.show_side_panel_button";
const char kLocationBarIsWide[] = "brave.location_bar_is_wide";
const char kReferralDownloadID[] = "brave.referral.download_id";
const char kReferralTimestamp[] = "brave.referral.timestamp";
const char kReferralAttemptTimestamp[] =
    "brave.referral.referral_attempt_timestamp";
const char kReferralAttemptCount[] = "brave.referral.referral_attempt_count";
const char kReferralHeaders[] = "brave.referral.headers";
const char kReferralAndroidFirstRunTimestamp[] =
    "brave.referral_android_first_run_timestamp";
const char kNoScriptControlType[] = "brave.no_script_default";
const char kShieldsAdvancedViewEnabled[] =
    "brave.shields.advanced_view_enabled";
const char kShieldsStatsBadgeVisible[] = "brave.shields.stats_badge_visible";
const char kAdControlType[] = "brave.ad_default";
const char kGoogleLoginControlType[] = "brave.google_login_default";
const char kWebTorrentEnabled[] = "brave.webtorrent_enabled";
const char kHangoutsEnabled[] = "brave.hangouts_enabled";
const char kIPFSCompanionEnabled[] = "brave.ipfs_companion_enabled";
const char kNewTabPageShowClock[] = "brave.new_tab_page.show_clock";
const char kNewTabPageClockFormat[] = "brave.new_tab_page.clock_format";
const char kNewTabPageShowStats[] = "brave.new_tab_page.show_stats";
const char kNewTabPageShowRewards[] = "brave.new_tab_page.show_rewards";
const char kNewTabPageShowBinance[] = "brave.new_tab_page.show_binance";
const char kNewTabPageShowGemini[] = "brave.new_tab_page.show_gemini";
const char kNewTabPageShowBraveTalk[] = "brave.new_tab_page.show_together";
const char kNewTabPageHideAllWidgets[] = "brave.new_tab_page.hide_all_widgets";
const char kNewTabPageShowsOptions[] = "brave.new_tab_page.shows_options";
const char kBraveTodayIntroDismissed[] = "brave.today.intro_dismissed";
const char kBinanceAccessToken[] = "brave.binance.access_token";
const char kBinanceRefreshToken[] = "brave.binance.refresh_token";
const char kAlwaysShowBookmarkBarOnNTP[] =
    "brave.always_show_bookmark_bar_on_ntp";
const char kBraveDarkMode[] = "brave.dark_mode";
const char kOtherBookmarksMigrated[] = "brave.other_bookmarks_migrated";
const char kBraveShieldsSettingsVersion[] = "brave.shields_settings_version";
const char kDefaultBrowserPromptEnabled[] =
    "brave.default_browser_prompt_enabled";

const char kWebDiscoveryEnabled[] = "brave.web_discovery_enabled";
const char kDontAskEnableWebDiscovery[] = "brave.dont_ask_enable_web_discovery";
const char kBraveSearchVisitCount[] = "brave.brave_search_visit_count";

const char kBraveGCMChannelStatus[] = "brave.gcm.channel_status";
const char kImportDialogExtensions[] = "import_dialog_extensions";
const char kImportDialogPayments[] = "import_dialog_payments";
const char kMRUCyclingEnabled[] = "brave.mru_cycling_enabled";
const char kTabsSearchShow[] = "brave.tabs_search_show";
const char kDontAskForCrashReporting[] = "brave.dont_ask_for_crash_reporting";
const char kEnableMediaRouterOnRestart[] =
    "brave.enable_media_router_on_restart";

#if BUILDFLAG(IS_ANDROID)
const char kDesktopModeEnabled[] = "brave.desktop_mode_enabled";
const char kPlayYTVideoInBrowserEnabled[] =
    "brave.play_yt_video_in_browser_enabled";
const char kBackgroundVideoPlaybackEnabled[] =
    "brave.background_video_playback";
const char kSafetynetCheckFailed[] = "safetynetcheck.failed";
const char kSafetynetStatus[] = "safetynet.status";
#endif

#if !BUILDFLAG(IS_ANDROID)
const char kEnableWindowClosingConfirm[] =
    "brave.enable_window_closing_confirm";
const char kEnableClosingLastTab[] = "brave.enable_closing_last_tab";
#endif

const char kDefaultBrowserLaunchingCount[] =
    "brave.default_browser.launching_count";

// deprecated
const char kBraveThemeType[] = "brave.theme.type";
const char kUseOverriddenBraveThemeType[] =
    "brave.theme.use_overridden_brave_theme_type";
const char kNewTabPageShowTopSites[] = "brave.new_tab_page.show_top_sites";

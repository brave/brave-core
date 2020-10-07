/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/pref_names.h"

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
const char kWeekOfInstallation[] = "brave.stats.week_of_installation";
const char kAdBlockCheckedDefaultRegion[] =
    "brave.ad_block.checked_default_region";
const char kAdBlockCustomFilters[] = "brave.ad_block.custom_filters";
const char kAdBlockRegionalFilters[] = "brave.ad_block.regional_filters";
const char kWidevineOptedIn[] = "brave.widevine_opted_in";
const char kWidevineInstalledVersion[] = "brave.widevine_installed_version";
const char kAskWidevineInstall[] = "brave.ask_widevine_install";
const char kUseAlternativeSearchEngineProvider[] =
    "brave.use_alternate_private_search_engine";
const char kAlternativeSearchEngineProviderInTor[] =
    "brave.alternate_private_search_engine_in_tor";
const char kBraveThemeType[] = "brave.theme.type";  // deprecated
const char kUseOverriddenBraveThemeType[] =
    "brave.theme.use_overridden_brave_theme_type";  // deprecated
const char kLocationBarIsWide[] = "brave.location_bar_is_wide";
const char kReferralDownloadID[] = "brave.referral.download_id";
const char kReferralTimestamp[] = "brave.referral.timestamp";
const char kReferralAttemptTimestamp[] =
    "brave.referral.referral_attempt_timestamp";
const char kReferralAttemptCount[] = "brave.referral.referral_attempt_count";
const char kReferralHeaders[] = "brave.referral.headers";
const char kReferralAndroidFirstRunTimestamp[] =
    "brave.referral_android_first_run_timestamp";
const char kHTTPSEVerywhereControlType[] = "brave.https_everywhere_default";
const char kNoScriptControlType[] = "brave.no_script_default";
const char kShieldsAdvancedViewEnabled[] =
    "brave.shields.advanced_view_enabled";
const char kShieldsStatsBadgeVisible[] =
    "brave.shields.stats_badge_visible";
const char kAdControlType[] = "brave.ad_default";
const char kGoogleLoginControlType[] = "brave.google_login_default";
const char kFBEmbedControlType[] = "brave.fb_embed_default";
const char kTwitterEmbedControlType[] = "brave.twitter_embed_default";
const char kLinkedInEmbedControlType[] = "brave.linkedin_embed_default";
const char kWebTorrentEnabled[] = "brave.webtorrent_enabled";
const char kHangoutsEnabled[] = "brave.hangouts_enabled";
const char kIPFSResolveMethod[] = "brave.ipfs.resolve_method";
const char kIPFSBinaryAvailable[] = "brave.ipfs.binary_available";
const char kIPFSCompanionEnabled[] = "brave.ipfs_companion_enabled";
const char kNewTabPageShowClock[] = "brave.new_tab_page.show_clock";
const char kNewTabPageClockFormat[] = "brave.new_tab_page.clock_format";
const char kNewTabPageShowTopSites[] = "brave.new_tab_page.show_top_sites";
const char kNewTabPageShowStats[] = "brave.new_tab_page.show_stats";
const char kNewTabPageShowRewards[] = "brave.new_tab_page.show_rewards";
const char kNewTabPageShowBinance[] = "brave.new_tab_page.show_binance";
const char kNewTabPageShowGemini[] = "brave.new_tab_page.show_gemini";
const char kNewTabPageShowTogether[] = "brave.new_tab_page.show_together";
const char kNewTabPageShowAddCard[] = "brave.new_tab_page.show_addcard";
const char kBraveEnabledMediaRouter[] = "brave.enable_media_router";
const char kBinanceAccessToken[] = "brave.binance.access_token";
const char kBinanceRefreshToken[] = "brave.binance.refresh_token";
const char kAlwaysShowBookmarkBarOnNTP[] =
    "brave.always_show_bookmark_bar_on_ntp";
const char kAutocompleteEnabled[] = "brave.autocomplete_enabled";
const char kTopSiteSuggestionsEnabled[] = "brave.top_site_suggestions_enabled";
const char kBraveSuggestedSiteSuggestionsEnabled[] =
    "brave.brave_suggested_site_suggestions_enabled";
const char kBraveDarkMode[] = "brave.dark_mode";
const char kOtherBookmarksMigrated[] = "brave.other_bookmarks_migrated";
const char kBraveShieldsSettingsVersion[] = "brave.shields_settings_version";
#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
const char kBraveGCMChannelStatus[] = "brave.gcm.channel_status";
#endif
const char kImportDialogExtensions[] = "import_dialog_extensions";
const char kImportDialogPayments[] = "import_dialog_payments";

#if defined(OS_ANDROID)
const char kDesktopModeEnabled[] = "brave.desktop_mode_enabled";
const char kPlayYTVideoInBrowserEnabled[] =
    "brave.play_yt_video_in_browser_enabled";
const char kBackgroundVideoPlaybackEnabled[] =
    "brave.background_video_playback";
const char kSafetynetCheckFailed[] = "safetynetcheck.failed";
const char kSafetynetStatus[] = "safetynet.status";
#endif

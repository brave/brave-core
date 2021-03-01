/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_PREF_NAMES_H_
#define BRAVE_COMMON_PREF_NAMES_H_

#include "build/build_config.h"
#include "components/gcm_driver/gcm_buildflags.h"

extern const char kAdsBlocked[];
extern const char kTrackersBlocked[];
extern const char kJavascriptBlocked[];
extern const char kHttpsUpgrades[];
extern const char kFingerprintingBlocked[];
extern const char kLastCheckYMD[];
extern const char kLastCheckWOY[];
extern const char kLastCheckMonth[];
extern const char kFirstCheckMade[];
extern const char kThresholdCheckMade[];
extern const char kThresholdQuery[];
extern const char kWeekOfInstallation[];
extern const char kAdBlockCheckedDefaultRegion[];
extern const char kAdBlockCustomFilters[];
extern const char kAdBlockRegionalFilters[];
extern const char kWidevineOptedIn[];
extern const char kAskWidevineInstall[];
extern const char kUseAlternativeSearchEngineProvider[];
extern const char kAlternativeSearchEngineProviderInTor[];
extern const char kBraveThemeType[];
extern const char kUseOverriddenBraveThemeType[];
extern const char kLocationBarIsWide[];
extern const char kReferralDownloadID[];
extern const char kReferralTimestamp[];
extern const char kReferralAttemptTimestamp[];
extern const char kReferralAttemptCount[];
extern const char kReferralHeaders[];
extern const char kReferralAndroidFirstRunTimestamp[];
extern const char kHTTPSEverywhereControlType[];
extern const char kNoScriptControlType[];
extern const char kShieldsAdvancedViewEnabled[];
extern const char kShieldsStatsBadgeVisible[];
extern const char kAdControlType[];
extern const char kGoogleLoginControlType[];
extern const char kFBEmbedControlType[];
extern const char kTwitterEmbedControlType[];
extern const char kLinkedInEmbedControlType[];
extern const char kWebTorrentEnabled[];
extern const char kHangoutsEnabled[];
extern const char kIPFSCompanionEnabled[];
extern const char kNewTabPageShowClock[];
extern const char kNewTabPageClockFormat[];
extern const char kNewTabPageShowTopSites[];
extern const char kNewTabPageShowStats[];
extern const char kNewTabPageShowToday[];
extern const char kNewTabPageShowRewards[];
extern const char kNewTabPageShowBinance[];
extern const char kNewTabPageShowGemini[];
extern const char kNewTabPageShowTogether[];
extern const char kNewTabPageShowsOptions[];
extern const char kBraveTodaySources[];
extern const char kBraveTodayIntroDismissed[];
extern const char kBraveTodayWeeklySessionCount[];
extern const char kBraveTodayWeeklyCardViewsCount[];
extern const char kBraveTodayWeeklyCardVisitsCount[];
extern const char kBraveEnabledMediaRouter[];
extern const char kAlwaysShowBookmarkBarOnNTP[];
extern const char kAutocompleteEnabled[];
extern const char kTopSiteSuggestionsEnabled[];
extern const char kBraveSuggestedSiteSuggestionsEnabled[];
extern const char kBraveDarkMode[];
extern const char kOtherBookmarksMigrated[];
extern const char kBraveShieldsSettingsVersion[];
extern const char kBinanceAccessToken[];
extern const char kBinanceRefreshToken[];
#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
extern const char kBraveGCMChannelStatus[];
#endif
extern const char kImportDialogExtensions[];
extern const char kImportDialogPayments[];
extern const char kMRUCyclingEnabled[];

#if defined(OS_ANDROID)
extern const char kDesktopModeEnabled[];
extern const char kPlayYTVideoInBrowserEnabled[];
extern const char kBackgroundVideoPlaybackEnabled[];
extern const char kSafetynetCheckFailed[];
extern const char kSafetynetStatus[];
#endif

extern const char kDefaultBrowserLaunchingCount[];
extern const char kBraveDefaultSearchVersion[];

#endif  // BRAVE_COMMON_PREF_NAMES_H_

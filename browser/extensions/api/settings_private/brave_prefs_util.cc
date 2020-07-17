/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/settings_private/brave_prefs_util.h"

#include "brave/common/pref_names.h"
#include "brave/components/brave_wayback_machine/buildflags.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "chrome/browser/extensions/api/settings_private/prefs_util.h"
#include "chrome/common/extensions/api/settings_private.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/omnibox/browser/omnibox_prefs.h"

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/pref_names.h"
#endif


namespace extensions {

using ntp_background_images::prefs::kNewTabPageSuperReferralThemesOption;
using
    ntp_background_images::prefs::kNewTabPageShowSponsoredImagesBackgroundImage;
using ntp_background_images::prefs::kNewTabPageShowBackgroundImage;

namespace settings_api = api::settings_private;

const PrefsUtil::TypedPrefMap& BravePrefsUtil::GetWhitelistedKeys() {
  // Static cache, similar to parent class
  static PrefsUtil::TypedPrefMap* s_brave_whitelist = nullptr;
  if (s_brave_whitelist)
    return *s_brave_whitelist;
  s_brave_whitelist = new PrefsUtil::TypedPrefMap();
  // Start with parent class whitelist
  const auto chromium_prefs = PrefsUtil::GetWhitelistedKeys();
  s_brave_whitelist->insert(chromium_prefs.begin(), chromium_prefs.end());
  // Add Brave values to the whitelist
  // import data
  (*s_brave_whitelist)[kImportDialogExtensions] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kImportDialogPayments] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  // Default Brave shields
  (*s_brave_whitelist)[kShieldsAdvancedViewEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kShieldsStatsBadgeVisible] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kAdControlType] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kHTTPSEVerywhereControlType] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kNoScriptControlType] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kGoogleLoginControlType] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kFBEmbedControlType] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kTwitterEmbedControlType] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kLinkedInEmbedControlType] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;

  // appearance prefs
  (*s_brave_whitelist)[kLocationBarIsWide] =
    settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kAutocompleteEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kTopSiteSuggestionsEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kBraveSuggestedSiteSuggestionsEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[brave_rewards::prefs::kHideBraveRewardsButton] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kAskWidevineInstall] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kNewTabPageSuperReferralThemesOption] =
      settings_api::PrefType::PREF_TYPE_NUMBER;
  // new tab prefs
  (*s_brave_whitelist)[kNewTabPageShowSponsoredImagesBackgroundImage] =
        settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kNewTabPageShowBackgroundImage] =
        settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kNewTabPageShowClock] =
        settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kNewTabPageShowTopSites] =
        settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kNewTabPageShowStats] =
        settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kNewTabPageShowRewards] =
        settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kNewTabPageShowBinance] =
        settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kNewTabPageShowTogether] =
        settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kNewTabPageShowAddCard] =
        settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kNewTabPageShowGemini] =
        settings_api::PrefType::PREF_TYPE_BOOLEAN;
  // Clear browsing data on exit prefs.
  (*s_brave_whitelist)[browsing_data::prefs::kDeleteBrowsingHistoryOnExit] =
    settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[browsing_data::prefs::kDeleteDownloadHistoryOnExit] =
    settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[browsing_data::prefs::kDeleteCacheOnExit] =
    settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[browsing_data::prefs::kDeleteCookiesOnExit] =
    settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[browsing_data::prefs::kDeletePasswordsOnExit] =
    settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[browsing_data::prefs::kDeleteFormDataOnExit] =
    settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[browsing_data::prefs::kDeleteSiteSettingsOnExit] =
    settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[browsing_data::prefs::kDeleteHostedAppsDataOnExit] =
    settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[kAlwaysShowBookmarkBarOnNTP] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  // WebTorrent pref
  (*s_brave_whitelist)[kWebTorrentEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  (*s_brave_whitelist)[kBraveWaybackMachineEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
#endif
  // Hangouts pref
  (*s_brave_whitelist)[kHangoutsEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  // IPFS Companion pref
  (*s_brave_whitelist)[kIPFSCompanionEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  // Brave Wallet pref
  (*s_brave_whitelist)[kBraveWalletWeb3Provider] =
      settings_api::PrefType::PREF_TYPE_NUMBER;
  (*s_brave_whitelist)[kLoadCryptoWalletsOnStartup] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  // Media Router Pref
  (*s_brave_whitelist)[kBraveEnabledMediaRouter] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  // Push Messaging Pref
  (*s_brave_whitelist)[kBraveGCMChannelStatus] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
#endif
  // Omnibox pref
  (*s_brave_whitelist)[omnibox::kPreventUrlElisionsInOmnibox] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;

  return *s_brave_whitelist;
}

}  // namespace extensions

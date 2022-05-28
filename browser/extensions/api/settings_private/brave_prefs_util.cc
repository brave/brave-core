/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/settings_private/brave_prefs_util.h"

#include "bat/ads/pref_names.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wayback_machine/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/crypto_dot_com/browser/buildflags/buildflags.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "brave/components/decentralized_dns/pref_names.h"
#include "brave/components/ftx/browser/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/sidebar/buildflags/buildflags.h"
#include "brave/components/speedreader/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/extensions/api/settings_private/prefs_util.h"
#include "chrome/common/extensions/api/settings_private.h"
#include "chrome/common/pref_names.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/omnibox/browser/omnibox_prefs.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/pref_names.h"
#endif

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/ipfs/pref_names.h"
#endif

#if BUILDFLAG(CRYPTO_DOT_COM_ENABLED)
#include "brave/components/crypto_dot_com/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SIDEBAR)
#include "brave/components/sidebar/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_pref_names.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_FTX)
#include "brave/components/ftx/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/pref_names.h"
#endif

namespace extensions {

using ntp_background_images::prefs::kNewTabPageShowBackgroundImage;
using ntp_background_images::prefs::
    kNewTabPageShowSponsoredImagesBackgroundImage;
using ntp_background_images::prefs::kNewTabPageSuperReferralThemesOption;

namespace settings_api = api::settings_private;

const PrefsUtil::TypedPrefMap& BravePrefsUtil::GetAllowlistedKeys() {
  // Static cache, similar to parent class
  static PrefsUtil::TypedPrefMap* s_brave_allowlist = nullptr;
  if (s_brave_allowlist)
    return *s_brave_allowlist;
  s_brave_allowlist = new PrefsUtil::TypedPrefMap();
  // Start with parent class allowlist
  const auto chromium_prefs = PrefsUtil::GetAllowlistedKeys();
  s_brave_allowlist->insert(chromium_prefs.begin(), chromium_prefs.end());
  // Add Brave values to the allowlist
  // import data
  (*s_brave_allowlist)[kImportDialogExtensions] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kImportDialogPayments] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  // Default Brave shields
  (*s_brave_allowlist)[kShieldsAdvancedViewEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kShieldsStatsBadgeVisible] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kAdControlType] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kHTTPSEVerywhereControlType] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kNoScriptControlType] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kGoogleLoginControlType] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[brave_shields::prefs::kFBEmbedControlType] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[brave_shields::prefs::kTwitterEmbedControlType] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[brave_shields::prefs::kLinkedInEmbedControlType] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[brave_shields::prefs::kReduceLanguageEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;

  // Rewards/Ads prefs
  (*s_brave_allowlist)[ads::prefs::kEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[ads::prefs::kAdsPerHour] =
      settings_api::PrefType::PREF_TYPE_NUMBER;
  (*s_brave_allowlist)[ads::prefs::kShouldAllowAdsSubdivisionTargeting] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[ads::prefs::kAdsSubdivisionTargetingCode] =
      settings_api::PrefType::PREF_TYPE_STRING;
  (*s_brave_allowlist)[brave_rewards::prefs::kEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[brave_rewards::prefs::kAutoContributeEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[brave_rewards::prefs::kAutoContributeAmount] =
      settings_api::PrefType::PREF_TYPE_NUMBER;
  (*s_brave_allowlist)[brave_rewards::prefs::kMinVisitTime] =
      settings_api::PrefType::PREF_TYPE_NUMBER;
  (*s_brave_allowlist)[brave_rewards::prefs::kMinVisits] =
      settings_api::PrefType::PREF_TYPE_NUMBER;
  (*s_brave_allowlist)[brave_rewards::prefs::kAllowNonVerified] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[brave_rewards::prefs::kAllowVideoContribution] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[brave_rewards::prefs::kInlineTipRedditEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[brave_rewards::prefs::kInlineTipTwitterEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[brave_rewards::prefs::kInlineTipGithubEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;

  // Search engine prefs
  (*s_brave_allowlist)[prefs::kAddOpenSearchEngines] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;

  // appearance prefs
  (*s_brave_allowlist)[kShowBookmarksButton] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kLocationBarIsWide] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kAutocompleteEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kTopSiteSuggestionsEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kBraveSuggestedSiteSuggestionsEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[brave_rewards::prefs::kShowButton] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kAskWidevineInstall] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kNewTabPageSuperReferralThemesOption] =
      settings_api::PrefType::PREF_TYPE_NUMBER;
  (*s_brave_allowlist)[kTabsSearchShow] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  (*s_brave_allowlist)[brave_vpn::prefs::kBraveVPNShowButton] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
#endif
#if BUILDFLAG(ENABLE_SIDEBAR)
  (*s_brave_allowlist)[sidebar::kSidebarShowOption] =
      settings_api::PrefType::PREF_TYPE_NUMBER;
#endif
#if BUILDFLAG(ENABLE_SPEEDREADER)
  (*s_brave_allowlist)[speedreader::kSpeedreaderPrefEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
#endif
  // De-AMP feature
  (*s_brave_allowlist)[de_amp::kDeAmpPrefEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  // new tab prefs
  (*s_brave_allowlist)[kNewTabPageShowSponsoredImagesBackgroundImage] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kNewTabPageShowBackgroundImage] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kNewTabPageShowClock] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kNewTabPageShowStats] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kNewTabPageShowRewards] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kNewTabPageShowBinance] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kNewTabPageShowBraveTalk] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kNewTabPageShowGemini] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kNewTabPageShowsOptions] =
      settings_api::PrefType::PREF_TYPE_NUMBER;
#if BUILDFLAG(CRYPTO_DOT_COM_ENABLED)
  (*s_brave_allowlist)[kCryptoDotComNewTabPageShowCryptoDotCom] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
#endif
#if BUILDFLAG(ENABLE_FTX)
  (*s_brave_allowlist)[kFTXNewTabPageShowFTX] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
#endif
#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Web discovery prefs
  (*s_brave_allowlist)[kWebDiscoveryEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
#endif
  // Clear browsing data on exit prefs.
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteBrowsingHistoryOnExit] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteDownloadHistoryOnExit] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteCacheOnExit] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteCookiesOnExit] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[browsing_data::prefs::kDeletePasswordsOnExit] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteFormDataOnExit] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteSiteSettingsOnExit] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteHostedAppsDataOnExit] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kAlwaysShowBookmarkBarOnNTP] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kMRUCyclingEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  // WebTorrent pref
  (*s_brave_allowlist)[kWebTorrentEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  (*s_brave_allowlist)[kBraveWaybackMachineEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
#endif
  (*s_brave_allowlist)[kEnableWindowClosingConfirm] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  // Hangouts pref
  (*s_brave_allowlist)[kHangoutsEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  // IPFS Companion pref
  (*s_brave_allowlist)[kIPFSCompanionEnabled] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;

  // Brave Wallet pref
  (*s_brave_allowlist)[kBraveWalletSelectedNetworks] =
      settings_api::PrefType::PREF_TYPE_DICTIONARY;
  (*s_brave_allowlist)[kDefaultWallet2] =
      settings_api::PrefType::PREF_TYPE_NUMBER;
  (*s_brave_allowlist)[kDefaultBaseCurrency] =
      settings_api::PrefType::PREF_TYPE_STRING;
  (*s_brave_allowlist)[kDefaultBaseCryptocurrency] =
      settings_api::PrefType::PREF_TYPE_STRING;
  (*s_brave_allowlist)[kShowWalletIconOnToolbar] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kShowWalletTestNetworks] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kBraveWalletAutoLockMinutes] =
      settings_api::PrefType::PREF_TYPE_NUMBER;

  // IPFS pref
#if BUILDFLAG(ENABLE_IPFS)
  (*s_brave_allowlist)[kIPFSResolveMethod] =
      settings_api::PrefType::PREF_TYPE_NUMBER;
  (*s_brave_allowlist)[kIPFSAutoFallbackToGateway] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kIPFSPublicGatewayAddress] =
      settings_api::PrefType::PREF_TYPE_STRING;
  (*s_brave_allowlist)[kIPFSAutoRedirectGateway] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kIPFSAutoRedirectDNSLink] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_allowlist)[kIpfsStorageMax] =
      settings_api::PrefType::PREF_TYPE_NUMBER;
#endif

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  // Push Messaging Pref
  (*s_brave_allowlist)[kBraveGCMChannelStatus] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
#endif
  // Omnibox pref
  (*s_brave_allowlist)[omnibox::kPreventUrlElisionsInOmnibox] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
#if BUILDFLAG(ENABLE_TOR)
  (*s_brave_allowlist)[tor::prefs::kAutoOnionRedirect] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
#endif
  (*s_brave_allowlist)[prefs::kWebRTCIPHandlingPolicy] =
      settings_api::PrefType::PREF_TYPE_STRING;

  (*s_brave_allowlist)[decentralized_dns::kUnstoppableDomainsResolveMethod] =
      settings_api::PrefType::PREF_TYPE_NUMBER;
  (*s_brave_allowlist)[decentralized_dns::kENSResolveMethod] =
      settings_api::PrefType::PREF_TYPE_NUMBER;

  // Media router pref
  (*s_brave_allowlist)[kEnableMediaRouterOnRestart] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;

  return *s_brave_allowlist;
}

}  // namespace extensions

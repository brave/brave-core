/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/settings_private/brave_prefs_util.h"

#include "base/feature_list.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "brave/components/debounce/core/common/pref_names.h"
#include "brave/components/decentralized_dns/core/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/request_otr/common/pref_names.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/api/settings_private/prefs_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/settings_private.h"
#include "chrome/common/pref_names.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/omnibox/browser/omnibox_prefs.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "extensions/buildflags/buildflags.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/pref_names.h"
#endif

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "brave/components/sidebar/browser/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_pref_names.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/components/playlist/browser/pref_names.h"
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
  if (s_brave_allowlist) {
    return *s_brave_allowlist;
  }
  s_brave_allowlist = new PrefsUtil::TypedPrefMap();
  // Start with parent class allowlist
  const auto chromium_prefs = PrefsUtil::GetAllowlistedKeys();
  s_brave_allowlist->insert(chromium_prefs.begin(), chromium_prefs.end());
  // Add Brave values to the allowlist
  // import data
  (*s_brave_allowlist)[kImportDialogExtensions] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kImportDialogPayments] =
      settings_api::PrefType::kBoolean;
  // Default Brave shields
  (*s_brave_allowlist)[kShieldsAdvancedViewEnabled] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kShieldsStatsBadgeVisible] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kAdControlType] = settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kNoScriptControlType] = settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kGoogleLoginControlType] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[brave_shields::prefs::kFBEmbedControlType] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[brave_shields::prefs::kTwitterEmbedControlType] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[brave_shields::prefs::kLinkedInEmbedControlType] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[brave_shields::prefs::kReduceLanguageEnabled] =
      settings_api::PrefType::kBoolean;

  // Rewards/Ads prefs
  (*s_brave_allowlist)[brave_rewards::prefs::kEnabled] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[brave_rewards::prefs::kShowLocationBarButton] =
      settings_api::PrefType::kBoolean;

  // Search engine prefs
  (*s_brave_allowlist)[prefs::kAddOpenSearchEngines] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[prefs::kSyncedDefaultPrivateSearchProviderGUID] =
      settings_api::PrefType::kNumber;

  // autofill prefs
  (*s_brave_allowlist)[kBraveAutofillPrivateWindows] =
      settings_api::PrefType::kBoolean;

  // appearance prefs
  (*s_brave_allowlist)[kShowBookmarksButton] = settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kShowSidePanelButton] = settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[brave_news::prefs::kShouldShowToolbarButton] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kLocationBarIsWide] = settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[omnibox::kAutocompleteEnabled] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[omnibox::kTopSiteSuggestionsEnabled] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[omnibox::kHistorySuggestionsEnabled] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[omnibox::kBookmarkSuggestionsEnabled] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[omnibox::kCommanderSuggestionsEnabled] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kAskEnableWidvine] = settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kNewTabPageSuperReferralThemesOption] =
      settings_api::PrefType::kNumber;
  (*s_brave_allowlist)[kTabsSearchShow] = settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[brave_tabs::kTabHoverMode] =
      settings_api::PrefType::kNumber;
  (*s_brave_allowlist)[kTabMuteIndicatorNotClickable] =
      settings_api::PrefType::kBoolean;
#if BUILDFLAG(IS_MAC)
  (*s_brave_allowlist)[prefs::kShowFullscreenToolbar] =
      settings_api::PrefType::kBoolean;
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  (*s_brave_allowlist)[brave_vpn::prefs::kBraveVPNShowButton] =
      settings_api::PrefType::kBoolean;
#if BUILDFLAG(ENABLE_BRAVE_VPN_WIREGUARD)
  (*s_brave_allowlist)[brave_vpn::prefs::kBraveVPNWireguardEnabled] =
      settings_api::PrefType::kBoolean;
#endif
#endif
#if defined(TOOLKIT_VIEWS)
  (*s_brave_allowlist)[sidebar::kSidebarShowOption] =
      settings_api::PrefType::kNumber;
#endif
#if BUILDFLAG(ENABLE_SPEEDREADER)
  (*s_brave_allowlist)[speedreader::kSpeedreaderPrefEnabled] =
      settings_api::PrefType::kBoolean;
#endif
  // De-AMP feature
  (*s_brave_allowlist)[de_amp::kDeAmpPrefEnabled] =
      settings_api::PrefType::kBoolean;
  // Debounce feature
  (*s_brave_allowlist)[debounce::prefs::kDebounceEnabled] =
      settings_api::PrefType::kBoolean;
  // new tab prefs
  (*s_brave_allowlist)[kNewTabPageShowSponsoredImagesBackgroundImage] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kNewTabPageShowBackgroundImage] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kNewTabPageShowClock] = settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kNewTabPageShowStats] = settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kNewTabPageShowRewards] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kNewTabPageShowBraveTalk] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kNewTabPageShowsOptions] =
      settings_api::PrefType::kNumber;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Web discovery prefs
  (*s_brave_allowlist)[kWebDiscoveryEnabled] = settings_api::PrefType::kBoolean;
#endif
  // Clear browsing data on exit prefs.
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteBrowsingHistoryOnExit] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteDownloadHistoryOnExit] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteCacheOnExit] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteCookiesOnExit] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[browsing_data::prefs::kDeletePasswordsOnExit] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteFormDataOnExit] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteSiteSettingsOnExit] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteHostedAppsDataOnExit] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteBraveLeoHistory] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[browsing_data::prefs::kDeleteBraveLeoHistoryOnExit] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kAlwaysShowBookmarkBarOnNTP] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kMRUCyclingEnabled] = settings_api::PrefType::kBoolean;
  // WebTorrent pref
  (*s_brave_allowlist)[kWebTorrentEnabled] = settings_api::PrefType::kBoolean;
#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  (*s_brave_allowlist)[kBraveWaybackMachineEnabled] =
      settings_api::PrefType::kBoolean;
#endif
  (*s_brave_allowlist)[kEnableWindowClosingConfirm] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kEnableClosingLastTab] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kShowFullscreenReminder] =
      settings_api::PrefType::kBoolean;

  // Brave Wallet pref
  (*s_brave_allowlist)[kBraveWalletSelectedNetworks] =
      settings_api::PrefType::kDictionary;
  (*s_brave_allowlist)[kDefaultEthereumWallet] =
      settings_api::PrefType::kNumber;
  (*s_brave_allowlist)[kDefaultSolanaWallet] = settings_api::PrefType::kNumber;
  (*s_brave_allowlist)[kDefaultBaseCurrency] = settings_api::PrefType::kString;
  (*s_brave_allowlist)[kDefaultBaseCryptocurrency] =
      settings_api::PrefType::kString;
  (*s_brave_allowlist)[kShowWalletIconOnToolbar] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kBraveWalletAutoLockMinutes] =
      settings_api::PrefType::kNumber;
  (*s_brave_allowlist)[kBraveWalletTransactionSimulationOptInStatus] =
      settings_api::PrefType::kNumber;
  (*s_brave_allowlist)[kBraveWalletNftDiscoveryEnabled] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[kBraveWalletPrivateWindowsEnabled] =
      settings_api::PrefType::kBoolean;

// Leo Assistant pref
#if BUILDFLAG(ENABLE_AI_CHAT)
  (*s_brave_allowlist)[ai_chat::prefs::kBraveChatAutocompleteProviderEnabled] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[ai_chat::prefs::kBraveAIChatContextMenuEnabled] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[ai_chat::prefs::kBraveAIChatShowToolbarButton] =
      settings_api::PrefType::kBoolean;
#endif

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  // Push Messaging Pref
  (*s_brave_allowlist)[kBraveGCMChannelStatus] =
      settings_api::PrefType::kBoolean;
#endif
  // Omnibox pref
  (*s_brave_allowlist)[omnibox::kPreventUrlElisionsInOmnibox] =
      settings_api::PrefType::kBoolean;
#if BUILDFLAG(ENABLE_TOR)
  (*s_brave_allowlist)[tor::prefs::kOnionOnlyInTorWindows] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[tor::prefs::kBridgesConfig] =
      settings_api::PrefType::kDictionary;
#endif
  (*s_brave_allowlist)[prefs::kWebRTCIPHandlingPolicy] =
      settings_api::PrefType::kString;
  // Request OTR feature
  (*s_brave_allowlist)[request_otr::kRequestOTRActionOption] =
      settings_api::PrefType::kNumber;

  (*s_brave_allowlist)[decentralized_dns::kUnstoppableDomainsResolveMethod] =
      settings_api::PrefType::kNumber;
  (*s_brave_allowlist)[decentralized_dns::kENSResolveMethod] =
      settings_api::PrefType::kNumber;
  (*s_brave_allowlist)[decentralized_dns::kEnsOffchainResolveMethod] =
      settings_api::PrefType::kNumber;
  (*s_brave_allowlist)[decentralized_dns::kSnsResolveMethod] =
      settings_api::PrefType::kNumber;

  // Media router pref
  (*s_brave_allowlist)[kEnableMediaRouterOnRestart] =
      settings_api::PrefType::kBoolean;

#if defined(TOOLKIT_VIEWS)
  // Vertical tab strip prefs
  (*s_brave_allowlist)[brave_tabs::kVerticalTabsEnabled] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[brave_tabs::kVerticalTabsFloatingEnabled] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[brave_tabs::kVerticalTabsShowTitleOnWindow] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[brave_tabs::kVerticalTabsOnRight] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[brave_tabs::kVerticalTabsShowScrollbar] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[brave_tabs::kVerticalTabsExpandedStatePerWindow] =
      settings_api::PrefType::kBoolean;
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
  (*s_brave_allowlist)[playlist::kPlaylistEnabledPref] =
      settings_api::PrefType::kBoolean;
  (*s_brave_allowlist)[playlist::kPlaylistCacheByDefault] =
      settings_api::PrefType::kBoolean;
#endif

#if !BUILDFLAG(IS_ANDROID)
  (*s_brave_allowlist)[brave_tabs::kSharedPinnedTab] =
      settings_api::PrefType::kBoolean;
#endif

  return *s_brave_allowlist;
}

std::optional<api::settings_private::PrefObject> BravePrefsUtil::GetPref(
    const std::string& name) {
  auto pref = PrefsUtil::GetPref(name);
  // Simulate "Enforced" mode for kCookieControlsMode pref when Cookies are
  // fully blocked via Shields. This will effectively disable the "Third-party
  // cookies" mode selector on Settings page.
  if (pref && name == prefs::kCookieControlsMode &&
      pref->enforcement == api::settings_private::Enforcement::kNone &&
      brave_shields::GetCookieControlType(
          HostContentSettingsMapFactory::GetForProfile(profile()),
          CookieSettingsFactory::GetForProfile(profile()).get(),
          GURL()) == brave_shields::ControlType::BLOCK) {
    pref->enforcement = api::settings_private::Enforcement::kEnforced;
  }
  return pref;
}

}  // namespace extensions

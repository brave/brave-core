/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_profile_prefs.h"

#include <string>

#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/browser/new_tab/new_tab_shows_options.h"
#include "brave/browser/search/ntp_utils.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/translate/brave_translate_prefs_migration.h"
#include "brave/browser/ui/bookmark/brave_bookmark_prefs.h"
#include "brave/browser/ui/omnibox/brave_omnibox_client_impl.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#include "brave/components/brave_ads/browser/analytics/p2a/p2a.h"
#include "brave/components/brave_ads/core/public/prefs/obsolete_pref_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/browser/brave_news_p3a.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/common/p3a_pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_perf_predictor/browser/p3a_bandwidth_savings_tracker.h"
#include "brave/components/brave_perf_predictor/browser/perf_predictor_tab_helper.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_registry.h"
#include "brave/components/brave_search/browser/brave_search_default_host.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/brave_shields/content/browser/brave_farbling_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "brave/components/debounce/core/browser/debounce_service.h"
#include "brave/components/ipfs/ipfs_prefs.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "brave/components/ntp_background_images/buildflags/buildflags.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/request_otr/common/buildflags/buildflags.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/components/webcompat_reporter/common/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/prefetch/pref_names.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/preloading/preloading_prefs.h"
#include "chrome/browser/ui/webui/new_tab_page/ntp_pref_names.h"
#include "chrome/common/pref_names.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/embedder_support/pref_names.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/privacy_sandbox/privacy_sandbox_prefs.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "components/spellcheck/browser/pref_names.h"
#include "components/sync/base/pref_names.h"
#include "extensions/buildflags/buildflags.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/components/brave_webtorrent/browser/webtorrent_util.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/pref_names.h"
#endif

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#include "brave/browser/ethereum_remote_client/pref_names.h"
#endif

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
#include "brave/browser/gcm_driver/brave_gcm_utils.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_service.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/tor_profile_service.h"
#endif

#if BUILDFLAG(IS_ANDROID)
#include "components/feed/core/common/pref_names.h"
#include "components/feed/core/shared_prefs/pref_names.h"
#include "components/ntp_tiles/pref_names.h"
#include "components/translate/core/browser/translate_pref_names.h"
#endif

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/components/brave_private_new_tab_ui/common/pref_names.h"
#include "chrome/browser/ui/webui/bookmarks/bookmark_prefs.h"
#include "chrome/browser/ui/webui/side_panel/bookmarks/bookmarks.mojom.h"
#endif

#if BUILDFLAG(ENABLE_REQUEST_OTR)
#include "brave/components/request_otr/browser/request_otr_service.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "brave/components/sidebar/browser/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/common/feature_switch.h"
using extensions::FeatureSwitch;
#endif

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
#include "brave/browser/ntp_background/ntp_background_prefs.h"
#endif

namespace brave {

void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
#if !BUILDFLAG(IS_ANDROID)
  new_tab_page::RegisterNewTabPagePrefsForMigration(registry);

  // Added 10/2022
  registry->RegisterIntegerPref(kDefaultBrowserLaunchingCount, 0);
#endif
  brave_wallet::RegisterProfilePrefsForMigration(registry);

  // Restore "Other Bookmarks" migration
  registry->RegisterBooleanPref(kOtherBookmarksMigrated, false);

  // Added 05/2021
  registry->RegisterBooleanPref(kBraveNewsIntroDismissed, false);

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Added 11/2022
  registry->RegisterBooleanPref(kDontAskEnableWebDiscovery, false);
  registry->RegisterIntegerPref(kBraveSearchVisitCount, 0);
#endif

  // Added 24/11/2022: https://github.com/brave/brave-core/pull/16027
#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
  registry->RegisterStringPref(kFTXAccessToken, "");
  registry->RegisterStringPref(kFTXOauthHost, "");
  registry->RegisterBooleanPref(kFTXNewTabPageShowFTX, false);
  registry->RegisterBooleanPref(kCryptoDotComNewTabPageShowCryptoDotCom, false);
  registry->RegisterBooleanPref(kCryptoDotComHasBoughtCrypto, false);
  registry->RegisterBooleanPref(kCryptoDotComHasInteracted, false);
  registry->RegisterStringPref(kGeminiAccessToken, "");
  registry->RegisterStringPref(kGeminiRefreshToken, "");
  registry->RegisterBooleanPref(kNewTabPageShowGemini, false);
#endif

  // Added 24/11/2022: https://github.com/brave/brave-core/pull/16027
#if !BUILDFLAG(IS_IOS)
  registry->RegisterStringPref(kBinanceAccessToken, "");
  registry->RegisterStringPref(kBinanceRefreshToken, "");
  registry->RegisterBooleanPref(kNewTabPageShowBinance, false);
  registry->RegisterBooleanPref(kBraveSuggestedSiteSuggestionsEnabled, false);
#endif

  // Added Feb 2023
  registry->RegisterBooleanPref(brave_rewards::prefs::kShowButton, true);

  brave_rewards::RegisterProfilePrefsForMigration(registry);

  brave_news::p3a::prefs::RegisterProfileNewsMetricsPrefsForMigration(registry);

  // Added May 2023
#if defined(TOOLKIT_VIEWS)
  registry->RegisterBooleanPref(sidebar::kSidebarAlignmentChangedTemporarily,
                                false);
#endif

  // Added 2023-09
  ntp_background_images::ViewCounterService::RegisterProfilePrefsForMigration(
      registry);

  // Added 2023-11
  brave_sync::Prefs::RegisterProfilePrefsForMigration(registry);

  // Added 2023-11
  brave_ads::RegisterProfilePrefsForMigration(registry);

  // Added 2024-04
  ai_chat::prefs::RegisterProfilePrefsForMigration(registry);

  brave_shields::RegisterShieldsP3AProfilePrefsForMigration(registry);

  // Added 2024-05
  ipfs::RegisterDeprecatedIpfsPrefs(registry);

  // Added 2024-07
  registry->RegisterBooleanPref(kHangoutsEnabled, false);
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  brave_shields::BraveShieldsWebContentsObserver::RegisterProfilePrefs(
      registry);

  brave_perf_predictor::PerfPredictorTabHelper::RegisterProfilePrefs(registry);
  brave_perf_predictor::P3ABandwidthSavingsTracker::RegisterProfilePrefs(
      registry);
  // autofill
  registry->RegisterBooleanPref(kBraveAutofillPrivateWindows, true);
  // appearance
  registry->RegisterBooleanPref(kShowBookmarksButton, true);
  registry->RegisterBooleanPref(kShowSidePanelButton, true);
  registry->RegisterBooleanPref(kLocationBarIsWide, false);
  registry->RegisterBooleanPref(kMRUCyclingEnabled, false);
  registry->RegisterBooleanPref(kTabsSearchShow, true);
  registry->RegisterBooleanPref(kTabMuteIndicatorNotClickable, false);

  brave_sync::Prefs::RegisterProfilePrefs(registry);

  brave_shields::RegisterShieldsP3AProfilePrefs(registry);

  brave_news::prefs::RegisterProfilePrefs(registry);

  // TODO(shong): Migrate this to local state also and guard in ENABLE_WIDEVINE.
  // We don't need to display "don't ask widevine prompt option" in settings
  // if widevine is disabled.
  // F/u issue: https://github.com/brave/brave-browser/issues/7000
  registry->RegisterBooleanPref(kAskEnableWidvine, true);

  // Default Brave shields
  registry->RegisterBooleanPref(kNoScriptControlType, false);
  registry->RegisterBooleanPref(kAdControlType, true);
  registry->RegisterBooleanPref(kShieldsAdvancedViewEnabled, false);

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  // PushMessaging
  gcm::RegisterGCMProfilePrefs(registry);
#endif

  registry->RegisterBooleanPref(kShieldsStatsBadgeVisible, true);
  registry->RegisterBooleanPref(kGoogleLoginControlType, true);
  registry->RegisterBooleanPref(brave_shields::prefs::kFBEmbedControlType,
                                true);
  registry->RegisterBooleanPref(brave_shields::prefs::kTwitterEmbedControlType,
                                true);
  registry->RegisterBooleanPref(brave_shields::prefs::kLinkedInEmbedControlType,
                                false);

  // WebTorrent
#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  webtorrent::RegisterProfilePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  registry->RegisterBooleanPref(kBraveWaybackMachineEnabled, true);
#endif

  brave_adaptive_captcha::BraveAdaptiveCaptchaService::RegisterProfilePrefs(
      registry);

#if BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(kDesktopModeEnabled, false);
  registry->RegisterBooleanPref(kPlayYTVideoInBrowserEnabled, true);
  registry->RegisterBooleanPref(kBackgroundVideoPlaybackEnabled, false);
  registry->RegisterBooleanPref(kSafetynetCheckFailed, false);
  // clear default popular sites
  registry->SetDefaultPrefValue(ntp_tiles::prefs::kPopularSitesJsonPref,
                                base::Value(base::Value::Type::LIST));
  // Disable NTP suggestions
  // On Android we want to have enable_feed_v2 parameter enabled to
  // provide linking with feed::FetchRssLinks at
  // BraveNewsTabHelper::DOMContentLoaded, but kEnableSnippets and
  // kArticlesListVisible must be defaulted to false to avoid failed assertion
  // at BraveNewTabPage.initializeMainView. So override
  // feed::prefs::RegisterFeedSharedProfilePrefs for Android only. Related
  // Chromium's commit: d3500b942cde04737bc13021173b6ffa11aaf1b9.
  registry->SetDefaultPrefValue(feed::prefs::kEnableSnippets,
                                base::Value(false));
  registry->SetDefaultPrefValue(feed::prefs::kArticlesListVisible,
                                base::Value(false));
  registry->SetDefaultPrefValue(feed::prefs::kEnableSnippetsByDse,
                                base::Value(false));

  // Explicitly disable safe browsing extended reporting by default in case they
  // change it in upstream.
  registry->SetDefaultPrefValue(prefs::kSafeBrowsingScoutReportingEnabled,
                                base::Value(false));
#endif

  // Restore last profile on restart
  registry->SetDefaultPrefValue(
      prefs::kRestoreOnStartup,
      base::Value(SessionStartupPref::kPrefValueLast));

  // Show download prompt by default
  registry->SetDefaultPrefValue(prefs::kPromptForDownload, base::Value(true));

  // Not using chrome's web service for resolving navigation errors
  registry->SetDefaultPrefValue(embedder_support::kAlternateErrorPagesEnabled,
                                base::Value(false));

  // Disable safebrowsing reporting
  registry->SetDefaultPrefValue(
      prefs::kSafeBrowsingExtendedReportingOptInAllowed, base::Value(false));

#if defined(TOOLKIT_VIEWS)
  // Disable side search by default.
  // Copied from side_search_prefs.cc because it's not exported.
  constexpr char kSideSearchEnabled[] = "side_search.enabled";
  registry->SetDefaultPrefValue(kSideSearchEnabled, base::Value(false));
#endif

  // Disable search suggestion
  registry->SetDefaultPrefValue(prefs::kSearchSuggestEnabled,
                                base::Value(false));

  // Disable "Use a prediction service to load pages more quickly"
  registry->SetDefaultPrefValue(
      prefetch::prefs::kNetworkPredictionOptions,
      base::Value(
          static_cast<int>(prefetch::NetworkPredictionOptions::kDisabled)));

  // Disable cloud print
  // Cloud Print: Don't allow this browser to act as Cloud Print server
  registry->SetDefaultPrefValue(prefs::kCloudPrintProxyEnabled,
                                base::Value(false));

  // Disable default webstore icons in topsites or apps.
  registry->SetDefaultPrefValue(policy::policy_prefs::kHideWebStoreIcon,
                                base::Value(true));

  // Disable Chromium's privacy sandbox
  registry->SetDefaultPrefValue(prefs::kPrivacySandboxApisEnabled,
                                base::Value(false));
  registry->SetDefaultPrefValue(prefs::kPrivacySandboxApisEnabledV2,
                                base::Value(false));

  // Disable ScrollToText (Copy link to text).
  registry->SetDefaultPrefValue(prefs::kScrollToTextFragmentEnabled,
                                base::Value(false));

  // Importer: selected data types
  registry->RegisterBooleanPref(kImportDialogExtensions, true);
  registry->RegisterBooleanPref(kImportDialogPayments, true);

  // New Tab Page
  registry->RegisterBooleanPref(kNewTabPageShowClock, false);
  registry->RegisterStringPref(kNewTabPageClockFormat, "");
  registry->RegisterBooleanPref(kNewTabPageShowStats, true);
  registry->RegisterBooleanPref(kNewTabPageShowRewards, true);
  registry->RegisterBooleanPref(kNewTabPageShowBraveTalk, true);
  registry->RegisterBooleanPref(kNewTabPageHideAllWidgets, false);

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  registry->RegisterBooleanPref(kNewTabPageShowBraveVPN, true);
#endif

// Private New Tab Page
#if !BUILDFLAG(IS_ANDROID)
  brave_private_new_tab::prefs::RegisterProfilePrefs(registry);
#endif

  registry->RegisterIntegerPref(
      kNewTabPageShowsOptions,
      static_cast<int>(NewTabPageShowsOptions::kDashboard));

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
  NTPBackgroundPrefs::RegisterPref(registry);
#endif

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
  registry->RegisterIntegerPref(kERCPrefVersion, 0);
  registry->RegisterStringPref(kERCAES256GCMSivNonce, "");
  registry->RegisterStringPref(kERCEncryptedSeed, "");
  registry->RegisterBooleanPref(kERCOptedIntoCryptoWallets, false);
#endif

  // Brave Wallet
  brave_wallet::RegisterProfilePrefs(registry);

  // Brave Search
  if (brave_search::IsDefaultAPIEnabled()) {
    brave_search::BraveSearchDefaultHost::RegisterProfilePrefs(registry);
  }

  // Restore default behaviour for Android until we figure out if we want this
  // option there.
#if BUILDFLAG(IS_ANDROID)
  bool allow_open_search_engines = true;
#else
  bool allow_open_search_engines = false;
#endif
  registry->RegisterBooleanPref(prefs::kAddOpenSearchEngines,
                                allow_open_search_engines);

  omnibox::RegisterBraveProfilePrefs(registry);

  // Do not mark Password Manager app menu item as new
  registry->SetDefaultPrefValue(
      password_manager::prefs::kPasswordsPrefWithNewLabelUsed,
      base::Value(true));

  // Password leak detection should be disabled
  registry->SetDefaultPrefValue(
      password_manager::prefs::kPasswordLeakDetectionEnabled,
      base::Value(false));
  registry->SetDefaultPrefValue(syncer::prefs::internal::kSyncPayments,
                                base::Value(false));

  // Default search engine version
  registry->RegisterIntegerPref(
      prefs::kBraveDefaultSearchVersion,
      TemplateURLPrepopulateData::kBraveCurrentDataVersion);

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Web discovery extension, default false
  registry->RegisterBooleanPref(kWebDiscoveryEnabled, false);
  registry->RegisterDictionaryPref(kWebDiscoveryCTAState);
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
  speedreader::SpeedreaderService::RegisterProfilePrefs(registry);
#endif

  de_amp::RegisterProfilePrefs(registry);
  debounce::DebounceService::RegisterProfilePrefs(registry);

#if BUILDFLAG(ENABLE_TOR)
  tor::TorProfileService::RegisterProfilePrefs(registry);
#endif

#if !BUILDFLAG(IS_ANDROID)
  BraveOmniboxClientImpl::RegisterProfilePrefs(registry);
  brave_ads::RegisterP2APrefs(registry);

  // Turn on most visited mode on NTP by default.
  // We can turn customization mode on when we have add-shortcut feature.
  registry->SetDefaultPrefValue(ntp_prefs::kNtpUseMostVisitedTiles,
                                base::Value(true));
  registry->RegisterBooleanPref(kEnableWindowClosingConfirm, true);
  registry->RegisterBooleanPref(kEnableClosingLastTab, true);
  registry->RegisterBooleanPref(kShowFullscreenReminder, true);

  brave_tabs::RegisterBraveProfilePrefs(registry);

  registry->SetDefaultPrefValue(
      bookmarks_webui::prefs::kBookmarksViewType,
      base::Value(static_cast<int>(side_panel::mojom::ViewType::kCompact)));
#endif

  ai_chat::prefs::RegisterProfilePrefs(registry);
  ai_chat::ModelService::RegisterProfilePrefs(registry);

  brave_search_conversion::RegisterPrefs(registry);

  // Enabled by default after fixing
  // https://github.com/brave/brave-browser/issues/18017
  // kEnableMediaRouterOnRestart is used to remember the user's choice.
  registry->SetDefaultPrefValue(prefs::kEnableMediaRouter, base::Value(true));
  registry->RegisterBooleanPref(kEnableMediaRouterOnRestart, true);

  BraveFarblingService::RegisterProfilePrefs(registry);

  RegisterProfilePrefsForMigration(registry);

  translate::RegisterBraveProfilePrefsForMigration(registry);

#if BUILDFLAG(ENABLE_REQUEST_OTR)
  request_otr::RequestOTRService::RegisterProfilePrefs(registry);
#endif

#if defined(TOOLKIT_VIEWS)
  bookmarks::prefs::RegisterProfilePrefs(registry);
#endif

  brave_ads::RegisterProfilePrefs(registry);
  brave_rewards::RegisterProfilePrefs(registry);

  registry->SetDefaultPrefValue(prefs::kSearchSuggestEnabled,
                                base::Value(false));

  webcompat_reporter::prefs::RegisterProfilePrefs(registry);
}

}  // namespace brave

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_profile_prefs.h"

#include <string>

#include "brave/browser/new_tab/new_tab_shows_options.h"

#include "brave/browser/search/ntp_utils.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/omnibox/brave_omnibox_client_impl.h"
#include "brave/common/pref_names.h"
#include "brave/components/binance/browser/buildflags/buildflags.h"
#include "brave/components/brave_ads/browser/ads_p2a.h"
#include "brave/components/brave_perf_predictor/browser/buildflags.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/brave_wayback_machine/buildflags.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "brave/components/crypto_dot_com/browser/buildflags/buildflags.h"
#include "brave/components/gemini/browser/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "brave/components/sidebar/buildflags/buildflags.h"
#include "brave/components/speedreader/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/net/prediction_options.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/common/pref_names.h"
#include "components/autofill/core/common/autofill_prefs.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/embedder_support/pref_names.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "components/sync/base/pref_names.h"
#include "extensions/buildflags/buildflags.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/components/brave_webtorrent/browser/webtorrent_util.h"
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/widevine/widevine_utils.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/pref_names.h"
#endif

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#endif

#if BUILDFLAG(IPFS_ENABLED)
#include "brave/components/ipfs/ipfs_service.h"
#endif

#if BUILDFLAG(GEMINI_ENABLED)
#include "brave/components/gemini/browser/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_PERF_PREDICTOR)
#include "brave/components/brave_perf_predictor/browser/p3a_bandwidth_savings_tracker.h"
#include "brave/components/brave_perf_predictor/browser/perf_predictor_tab_helper.h"
#endif

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
#include "brave/browser/gcm_driver/brave_gcm_utils.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_service.h"
#endif

#if BUILDFLAG(CRYPTO_DOT_COM_ENABLED)
#include "brave/components/crypto_dot_com/browser/crypto_dot_com_pref_utils.h"
#include "brave/components/crypto_dot_com/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/tor_profile_service.h"
#endif

#if defined(OS_ANDROID)
#include "components/feed/core/shared_prefs/pref_names.h"
#include "components/ntp_tiles/pref_names.h"
#include "components/translate/core/browser/translate_pref_names.h"
#endif

#if !defined(OS_ANDROID)
#include "brave/browser/ui/startup/default_brave_browser_prompt.h"
#endif

#if BUILDFLAG(ENABLE_SIDEBAR)
#include "brave/components/sidebar/sidebar_service.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/common/feature_switch.h"
using extensions::FeatureSwitch;
#endif

namespace brave {

void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
#if BUILDFLAG(ENABLE_WIDEVINE)
  RegisterWidevineProfilePrefsForMigration(registry);
#endif

  dark_mode::RegisterBraveDarkModePrefsForMigration(registry);
#if !defined(OS_ANDROID)
  new_tab_page::RegisterNewTabPagePrefsForMigration(registry);
#endif

  // Restore "Other Bookmarks" migration
  registry->RegisterBooleanPref(kOtherBookmarksMigrated, false);

  // Added 04/2021
  registry->RegisterIntegerPref(
      kAlternativeSearchEngineProviderInTor,
      TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_INVALID);
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  brave_shields::BraveShieldsWebContentsObserver::RegisterProfilePrefs(
      registry);

#if BUILDFLAG(ENABLE_BRAVE_PERF_PREDICTOR)
  brave_perf_predictor::PerfPredictorTabHelper::RegisterProfilePrefs(registry);
  brave_perf_predictor::P3ABandwidthSavingsTracker::RegisterPrefs(registry);
#endif

  // appearance
  registry->RegisterBooleanPref(kLocationBarIsWide, false);
  registry->RegisterBooleanPref(brave_rewards::prefs::kHideButton, false);
  registry->RegisterBooleanPref(kMRUCyclingEnabled, false);

  brave_sync::Prefs::RegisterProfilePrefs(registry);

  // TODO(shong): Migrate this to local state also and guard in ENABLE_WIDEVINE.
  // We don't need to display "don't ask widevine prompt option" in settings
  // if widevine is disabled.
  // F/u issue: https://github.com/brave/brave-browser/issues/7000
  registry->RegisterBooleanPref(kAskWidevineInstall, true);

  // Default Brave shields
  registry->RegisterBooleanPref(kHTTPSEVerywhereControlType, true);
  registry->RegisterBooleanPref(kNoScriptControlType, false);
  registry->RegisterBooleanPref(kAdControlType, true);
  registry->RegisterBooleanPref(kShieldsAdvancedViewEnabled, false);

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  // PushMessaging
  gcm::RegisterGCMProfilePrefs(registry);
#endif

  registry->RegisterBooleanPref(kShieldsStatsBadgeVisible, true);
  registry->RegisterBooleanPref(kGoogleLoginControlType, true);
  registry->RegisterBooleanPref(kFBEmbedControlType, true);
  registry->RegisterBooleanPref(kTwitterEmbedControlType, true);
  registry->RegisterBooleanPref(kLinkedInEmbedControlType, false);

#if BUILDFLAG(IPFS_ENABLED)
  ipfs::IpfsService::RegisterPrefs(registry);
#endif

  // WebTorrent
#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  webtorrent::RegisterProfilePrefs(registry);
#endif

  // wayback machine
#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  registry->RegisterBooleanPref(kBraveWaybackMachineEnabled, true);
#endif

#if defined(OS_ANDROID)
  registry->RegisterBooleanPref(kDesktopModeEnabled, false);
  registry->RegisterBooleanPref(kPlayYTVideoInBrowserEnabled, true);
  registry->RegisterBooleanPref(kBackgroundVideoPlaybackEnabled, false);
  registry->RegisterBooleanPref(kSafetynetCheckFailed, false);
  // clear default popular sites
  registry->SetDefaultPrefValue(ntp_tiles::prefs::kPopularSitesJsonPref,
                                base::Value(base::Value::Type::LIST));
  // Disable NTP suggestions
  registry->SetDefaultPrefValue(feed::prefs::kEnableSnippets,
                                base::Value(false));
  registry->SetDefaultPrefValue(feed::prefs::kArticlesListVisible,
                                base::Value(false));
  // Translate is not available on Android
  registry->SetDefaultPrefValue(prefs::kOfferTranslateEnabled,
                                base::Value(false));
  // Explicitly disable safe browsing extended reporting by default in case they
  // change it in upstream.
  registry->SetDefaultPrefValue(prefs::kSafeBrowsingScoutReportingEnabled,
                                base::Value(false));
#endif

  // Hangouts
  registry->RegisterBooleanPref(kHangoutsEnabled, true);

  // Media Router
  registry->SetDefaultPrefValue(prefs::kEnableMediaRouter, base::Value(false));

  // 1. We do not want to enable the MediaRouter pref directly, so
  // using a proxy pref to handle Media Router setting
  // 2. On upgrade users might have enabled Media Router and the pref should
  // be set correctly, so we use feature switch to set the initial value
#if BUILDFLAG(ENABLE_EXTENSIONS)
  registry->RegisterBooleanPref(
      kBraveEnabledMediaRouter,
      FeatureSwitch::load_media_router_component_extension()->IsEnabled());
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

  // Disable search suggestion
  registry->SetDefaultPrefValue(prefs::kSearchSuggestEnabled,
                                base::Value(false));

  // Disable "Use a prediction service to load pages more quickly"
  registry->SetDefaultPrefValue(
      prefs::kNetworkPredictionOptions,
      base::Value(chrome_browser_net::NETWORK_PREDICTION_NEVER));

  // Disable cloud print
  // Cloud Print: Don't allow this browser to act as Cloud Print server
  registry->SetDefaultPrefValue(prefs::kCloudPrintProxyEnabled,
                                base::Value(false));
  // Cloud Print: Don't allow jobs to be submitted
  registry->SetDefaultPrefValue(prefs::kCloudPrintSubmitEnabled,
                                base::Value(false));

  // Disable default webstore icons in topsites or apps.
  registry->SetDefaultPrefValue(prefs::kHideWebStoreIcon, base::Value(true));

  // Importer: selected data types
  registry->RegisterBooleanPref(kImportDialogExtensions, true);
  registry->RegisterBooleanPref(kImportDialogPayments, true);

  // IPFS companion extension
  registry->RegisterBooleanPref(kIPFSCompanionEnabled, false);

  // New Tab Page
  registry->RegisterBooleanPref(kNewTabPageShowClock, true);
  registry->RegisterStringPref(kNewTabPageClockFormat, "");
  registry->RegisterBooleanPref(kNewTabPageShowStats, true);

  // Only default brave today to enabled for
  // english-language on browser startup.
  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  const std::string language_code = brave_l10n::GetLanguageCode(locale);
  const bool is_english_language = language_code == "en";
  const bool is_japanese_language = language_code == "ja";
  const bool brave_today_enabled_default = is_english_language ||
      is_japanese_language;
  registry->RegisterBooleanPref(kNewTabPageShowToday,
      brave_today_enabled_default);
  registry->RegisterBooleanPref(kNewTabPageShowRewards, true);
  registry->RegisterBooleanPref(kNewTabPageShowBinance, true);
  registry->RegisterBooleanPref(kNewTabPageShowTogether, false);
  registry->RegisterBooleanPref(kNewTabPageShowGemini, true);
  registry->RegisterIntegerPref(
      kNewTabPageShowsOptions,
      static_cast<int>(NewTabPageShowsOptions::kDashboard));

  // Brave Today
  registry->RegisterDictionaryPref(kBraveTodaySources);
  registry->RegisterBooleanPref(kBraveTodayIntroDismissed, false);
  registry->RegisterListPref(kBraveTodayWeeklySessionCount);
  registry->RegisterListPref(kBraveTodayWeeklyCardViewsCount);
  registry->RegisterListPref(kBraveTodayWeeklyCardVisitsCount);

  // Brave Wallet
#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  registry->RegisterIntegerPref(kBraveWalletPrefVersion, 0);
  registry->RegisterStringPref(kBraveWalletAES256GCMSivNonce, "");
  registry->RegisterStringPref(kBraveWalletEncryptedSeed, "");
  registry->RegisterIntegerPref(
      kBraveWalletWeb3Provider,
      static_cast<int>(brave_wallet::IsNativeWalletEnabled()
                           ? BraveWalletWeb3ProviderTypes::BRAVE_WALLET
                           : BraveWalletWeb3ProviderTypes::ASK));
  registry->RegisterBooleanPref(kLoadCryptoWalletsOnStartup, false);
  registry->RegisterBooleanPref(kOptedIntoCryptoWallets, false);
#endif

  // Binance widget
#if BUILDFLAG(BINANCE_ENABLED)
  registry->RegisterStringPref(kBinanceAccessToken, "");
  registry->RegisterStringPref(kBinanceRefreshToken, "");
#endif

  // Gemini widget
#if BUILDFLAG(GEMINI_ENABLED)
  registry->RegisterStringPref(kGeminiAccessToken, "");
  registry->RegisterStringPref(kGeminiRefreshToken, "");
#endif

  // Autocomplete in address bar
  registry->RegisterBooleanPref(kAutocompleteEnabled, true);
  registry->RegisterBooleanPref(kTopSiteSuggestionsEnabled, true);
  registry->RegisterBooleanPref(kBraveSuggestedSiteSuggestionsEnabled, false);

  // Password leak detection should be disabled
  registry->SetDefaultPrefValue(
      password_manager::prefs::kPasswordLeakDetectionEnabled,
      base::Value(false));
  registry->SetDefaultPrefValue(autofill::prefs::kAutofillWalletImportEnabled,
                                base::Value(false));

  // Default search engine version
  registry->RegisterIntegerPref(
      kBraveDefaultSearchVersion,
      TemplateURLPrepopulateData::kBraveCurrentDataVersion);

#if BUILDFLAG(ENABLE_SPEEDREADER)
  speedreader::SpeedreaderService::RegisterPrefs(registry);
#endif

#if BUILDFLAG(CRYPTO_DOT_COM_ENABLED)
  crypto_dot_com::RegisterPrefs(registry);
#endif

#if BUILDFLAG(ENABLE_TOR)
  tor::TorProfileService::RegisterPrefs(registry);
#endif

#if BUILDFLAG(ENABLE_SIDEBAR)
  sidebar::SidebarService::RegisterPrefs(registry);
#endif

#if !defined(OS_ANDROID)
  BraveOmniboxClientImpl::RegisterPrefs(registry);
#endif

#if !defined(OS_ANDROID)
  brave_ads::RegisterP2APrefs(registry);
#endif

#if !defined(OS_ANDROID)
  // Turn on most visited mode on NTP by default.
  // We can turn customization mode on when we have add-shortcut feature.
  registry->SetDefaultPrefValue(prefs::kNtpUseMostVisitedTiles,
                                base::Value(true));
  RegisterDefaultBraveBrowserPromptPrefs(registry);
#endif

  RegisterProfilePrefsForMigration(registry);
}

}  // namespace brave

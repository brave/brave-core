/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_local_state_prefs.h"
#include "brave/browser/brave_profile_prefs.h"
#include "brave/browser/brave_rewards/rewards_prefs_util.h"
#include "brave/browser/brave_stats/brave_stats_updater.h"
#include "brave/browser/search/ntp_utils.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/translate/brave_translate_prefs_migration.h"
#include "brave/components/brave_news/browser/brave_news_p3a.h"
#include "brave/components/brave_search_conversion/p3a.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "brave/components/ntp_background_images/buildflags/buildflags.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/translate/core/browser/translate_prefs.h"
#include "extensions/buildflags/buildflags.h"
#include "third_party/widevine/cdm/buildflags.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/search_engines/search_engine_provider_util.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/tor_utils.h"
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/widevine/widevine_utils.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN) && BUILDFLAG(IS_WIN)
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#endif

#if !BUILDFLAG(ENABLE_EXTENSIONS)
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_
#endif  // !BUILDFLAG(ENABLE_EXTENSIONS)

#define MigrateObsoleteProfilePrefs MigrateObsoleteProfilePrefs_ChromiumImpl
#define MigrateObsoleteLocalStatePrefs \
  MigrateObsoleteLocalStatePrefs_ChromiumImpl
#include "src/chrome/browser/prefs/browser_prefs.cc"
#undef MigrateObsoleteProfilePrefs
#undef MigrateObsoleteLocalStatePrefs

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
#include "brave/browser/gcm_driver/brave_gcm_utils.h"
#endif

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
#include "brave/browser/ntp_background/ntp_background_prefs.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "brave/components/sidebar/pref_names.h"
#endif

// This method should be periodically pruned of year+ old migrations.
void MigrateObsoleteProfilePrefs(Profile* profile) {
  // BEGIN_MIGRATE_OBSOLETE_PROFILE_PREFS
#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  // Added 02/2020.
  // Must be called before ChromiumImpl because it's migrating a Chromium pref
  // to Brave pref.
  gcm::MigrateGCMPrefs(profile);
#endif

  MigrateObsoleteProfilePrefs_ChromiumImpl(profile);

#if BUILDFLAG(ENABLE_WIDEVINE)
  // Added 11/2019.
  MigrateWidevinePrefs(profile);
#endif
  brave_sync::MigrateBraveSyncPrefs(profile->GetPrefs());

  // Added 12/2019.
  dark_mode::MigrateBraveDarkModePrefs(profile);

#if !BUILDFLAG(IS_ANDROID)
  // Added 9/2020
  new_tab_page::MigrateNewTabPagePrefs(profile);

  // Added 06/2022
  brave::MigrateSearchEngineProviderPrefs(profile);

  // Added 10/2022
  profile->GetPrefs()->ClearPref(kDefaultBrowserLaunchingCount);
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Added 11/2022
  profile->GetPrefs()->ClearPref(kDontAskEnableWebDiscovery);
  profile->GetPrefs()->ClearPref(kBraveSearchVisitCount);
#endif

  brave_wallet::KeyringService::MigrateObsoleteProfilePrefs(
      profile->GetPrefs());
  brave_wallet::MigrateObsoleteProfilePrefs(profile->GetPrefs());

  // Added 05/2021
  profile->GetPrefs()->ClearPref(kBraveNewsIntroDismissed);
  // Added 07/2021
  profile->GetPrefs()->ClearPref(prefs::kNetworkPredictionOptions);

  // Added 01/2022
  brave_rewards::MigrateObsoleteProfilePrefs(profile->GetPrefs());

  // Added 05/2022
  translate::ClearMigrationBraveProfilePrefs(profile->GetPrefs());

  // Added 06/2022
#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
  NTPBackgroundPrefs(profile->GetPrefs()).MigrateOldPref();
#endif

  // Added 24/11/2022: https://github.com/brave/brave-core/pull/16027
#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
  profile->GetPrefs()->ClearPref(kFTXAccessToken);
  profile->GetPrefs()->ClearPref(kFTXOauthHost);
  profile->GetPrefs()->ClearPref(kFTXNewTabPageShowFTX);
  profile->GetPrefs()->ClearPref(kCryptoDotComNewTabPageShowCryptoDotCom);
  profile->GetPrefs()->ClearPref(kCryptoDotComHasBoughtCrypto);
  profile->GetPrefs()->ClearPref(kCryptoDotComHasInteracted);
  profile->GetPrefs()->ClearPref(kGeminiAccessToken);
  profile->GetPrefs()->ClearPref(kGeminiRefreshToken);
  profile->GetPrefs()->ClearPref(kNewTabPageShowGemini);
#endif

  // Added 24/11/2022: https://github.com/brave/brave-core/pull/16027
#if !BUILDFLAG(IS_IOS)
  profile->GetPrefs()->ClearPref(kBinanceAccessToken);
  profile->GetPrefs()->ClearPref(kBinanceRefreshToken);
  profile->GetPrefs()->ClearPref(kNewTabPageShowBinance);
  profile->GetPrefs()->ClearPref(kBraveSuggestedSiteSuggestionsEnabled);
#endif

#if defined(TOOLKIT_VIEWS)
  // Added May 2023
  if (profile->GetPrefs()->GetBoolean(
          sidebar::kSidebarAlignmentChangedTemporarily)) {
    // If temporarily changed, it means sidebar is set to right.
    // Just clear alignment prefs as default alignment is changed to right.
    profile->GetPrefs()->ClearPref(prefs::kSidePanelHorizontalAlignment);
  }

  profile->GetPrefs()->ClearPref(sidebar::kSidebarAlignmentChangedTemporarily);
#endif

  brave_news::p3a::MigrateObsoleteProfilePrefs(profile->GetPrefs());

  // Added September 2023
#if !BUILDFLAG(IS_IOS)
  // TODO(https://github.com/brave/brave-browser/issues/33144): Remove after
  // several browser releases.
  constexpr const char* const kLegacyBraveP2AAdPrefs[] = {
      "Brave.P2A.TotalAdOpportunities",
      "Brave.P2A.AdOpportunitiesPerSegment.architecture",
      "Brave.P2A.AdOpportunitiesPerSegment.artsentertainment",
      "Brave.P2A.AdOpportunitiesPerSegment.automotive",
      "Brave.P2A.AdOpportunitiesPerSegment.business",
      "Brave.P2A.AdOpportunitiesPerSegment.careers",
      "Brave.P2A.AdOpportunitiesPerSegment.cellphones",
      "Brave.P2A.AdOpportunitiesPerSegment.crypto",
      "Brave.P2A.AdOpportunitiesPerSegment.education",
      "Brave.P2A.AdOpportunitiesPerSegment.familyparenting",
      "Brave.P2A.AdOpportunitiesPerSegment.fashion",
      "Brave.P2A.AdOpportunitiesPerSegment.folklore",
      "Brave.P2A.AdOpportunitiesPerSegment.fooddrink",
      "Brave.P2A.AdOpportunitiesPerSegment.gaming",
      "Brave.P2A.AdOpportunitiesPerSegment.healthfitness",
      "Brave.P2A.AdOpportunitiesPerSegment.history",
      "Brave.P2A.AdOpportunitiesPerSegment.hobbiesinterests",
      "Brave.P2A.AdOpportunitiesPerSegment.home",
      "Brave.P2A.AdOpportunitiesPerSegment.law",
      "Brave.P2A.AdOpportunitiesPerSegment.military",
      "Brave.P2A.AdOpportunitiesPerSegment.other",
      "Brave.P2A.AdOpportunitiesPerSegment.personalfinance",
      "Brave.P2A.AdOpportunitiesPerSegment.pets",
      "Brave.P2A.AdOpportunitiesPerSegment.realestate",
      "Brave.P2A.AdOpportunitiesPerSegment.science",
      "Brave.P2A.AdOpportunitiesPerSegment.sports",
      "Brave.P2A.AdOpportunitiesPerSegment.technologycomputing",
      "Brave.P2A.AdOpportunitiesPerSegment.travel",
      "Brave.P2A.AdOpportunitiesPerSegment.weather",
      "Brave.P2A.AdOpportunitiesPerSegment.untargeted",
      "Brave.P2A.TotalAdImpressions",
      "Brave.P2A.AdImpressionsPerSegment.architecture",
      "Brave.P2A.AdImpressionsPerSegment.artsentertainment",
      "Brave.P2A.AdImpressionsPerSegment.automotive",
      "Brave.P2A.AdImpressionsPerSegment.business",
      "Brave.P2A.AdImpressionsPerSegment.careers",
      "Brave.P2A.AdImpressionsPerSegment.cellphones",
      "Brave.P2A.AdImpressionsPerSegment.crypto",
      "Brave.P2A.AdImpressionsPerSegment.education",
      "Brave.P2A.AdImpressionsPerSegment.familyparenting",
      "Brave.P2A.AdImpressionsPerSegment.fashion",
      "Brave.P2A.AdImpressionsPerSegment.folklore",
      "Brave.P2A.AdImpressionsPerSegment.fooddrink",
      "Brave.P2A.AdImpressionsPerSegment.gaming",
      "Brave.P2A.AdImpressionsPerSegment.healthfitness",
      "Brave.P2A.AdImpressionsPerSegment.history",
      "Brave.P2A.AdImpressionsPerSegment.hobbiesinterests",
      "Brave.P2A.AdImpressionsPerSegment.home",
      "Brave.P2A.AdImpressionsPerSegment.law",
      "Brave.P2A.AdImpressionsPerSegment.military",
      "Brave.P2A.AdImpressionsPerSegment.other",
      "Brave.P2A.AdImpressionsPerSegment.personalfinance",
      "Brave.P2A.AdImpressionsPerSegment.pets",
      "Brave.P2A.AdImpressionsPerSegment.realestate",
      "Brave.P2A.AdImpressionsPerSegment.science",
      "Brave.P2A.AdImpressionsPerSegment.sports",
      "Brave.P2A.AdImpressionsPerSegment.technologycomputing",
      "Brave.P2A.AdImpressionsPerSegment.travel",
      "Brave.P2A.AdImpressionsPerSegment.weather",
      "Brave.P2A.AdImpressionsPerSegment.untargeted"};
  for (const char* const pref : kLegacyBraveP2AAdPrefs) {
    profile->GetPrefs()->ClearPref(pref);
  }
#endif

  // Added 2023-09
  ntp_background_images::ViewCounterService::MigrateObsoleteProfilePrefs(
      profile->GetPrefs());

  // END_MIGRATE_OBSOLETE_PROFILE_PREFS
}

// This method should be periodically pruned of year+ old migrations.
void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  // BEGIN_MIGRATE_OBSOLETE_LOCAL_STATE_PREFS
  MigrateObsoleteLocalStatePrefs_ChromiumImpl(local_state);

#if BUILDFLAG(ENABLE_WIDEVINE)
  // Added 11/2020.
  MigrateObsoleteWidevineLocalStatePrefs(local_state);
#endif

#if BUILDFLAG(ENABLE_TOR)
  // Added 4/2021.
  tor::MigrateLastUsedProfileFromLocalStatePrefs(local_state);
#endif

  decentralized_dns::MigrateObsoleteLocalStatePrefs(local_state);

#if BUILDFLAG(ENABLE_BRAVE_VPN) && BUILDFLAG(IS_WIN)
  // Migrating the feature flag here because dependencies relying on its value.
  brave_vpn::MigrateWireguardFeatureFlag(local_state);
#endif

#if !BUILDFLAG(IS_ANDROID)
  // Added 10/2022
  local_state->ClearPref(kDefaultBrowserPromptEnabled);
#endif

  brave_search_conversion::p3a::MigrateObsoleteLocalStatePrefs(local_state);
  brave_stats::MigrateObsoleteLocalStatePrefs(local_state);

  // END_MIGRATE_OBSOLETE_LOCAL_STATE_PREFS
}

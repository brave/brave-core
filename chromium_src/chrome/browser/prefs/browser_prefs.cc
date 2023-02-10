/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_local_state_prefs.h"
#include "brave/browser/brave_profile_prefs.h"
#include "brave/browser/brave_rewards/rewards_prefs_util.h"
#include "brave/browser/search/ntp_utils.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/translate/brave_translate_prefs_migration.h"
#include "brave/components/brave_news/browser/brave_news_p3a.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/decentralized_dns/core/utils.h"
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

#if !BUILDFLAG(IS_ANDROID)
  // Added 10/2022
  local_state->ClearPref(kDefaultBrowserPromptEnabled);
#endif

  // END_MIGRATE_OBSOLETE_LOCAL_STATE_PREFS
}

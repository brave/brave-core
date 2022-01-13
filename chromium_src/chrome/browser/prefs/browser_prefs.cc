/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_local_state_prefs.h"
#include "brave/browser/brave_profile_prefs.h"
#include "brave/browser/brave_rewards/rewards_prefs_util.h"
#include "brave/browser/search/ntp_utils.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/translate/core/browser/translate_prefs.h"
#include "extensions/buildflags/buildflags.h"
#include "third_party/widevine/cdm/buildflags.h"

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

// This method should be periodically pruned of year+ old migrations.
void MigrateObsoleteProfilePrefs(Profile* profile) {
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

  // Added 9/2020
#if !defined(OS_ANDROID)
  new_tab_page::MigrateNewTabPagePrefs(profile);
#endif

  brave_wallet::KeyringService::MigrateObsoleteProfilePrefs(
      profile->GetPrefs());
  brave_wallet::MigrateObsoleteProfilePrefs(profile->GetPrefs());

  // Added 04/2021
  profile->GetPrefs()->ClearPref(kAlternativeSearchEngineProviderInTor);
  // Added 05/2021
  profile->GetPrefs()->ClearPref(kBraveTodayIntroDismissed);
  // Added 07/2021
  profile->GetPrefs()->ClearPref(prefs::kNetworkPredictionOptions);

  // Added 01/2022
  brave_rewards::MigrateObsoleteProfilePrefs(profile->GetPrefs());
}

// This method should be periodically pruned of year+ old migrations.
void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  MigrateObsoleteLocalStatePrefs_ChromiumImpl(local_state);

#if BUILDFLAG(ENABLE_WIDEVINE)
  // Added 11/2020.
  MigrateObsoleteWidevineLocalStatePrefs(local_state);
#endif

#if BUILDFLAG(ENABLE_TOR)
  // Added 4/2021.
  tor::MigrateLastUsedProfileFromLocalStatePrefs(local_state);
#endif
}

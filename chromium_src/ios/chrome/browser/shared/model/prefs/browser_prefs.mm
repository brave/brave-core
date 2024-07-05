/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_ads/core/public/prefs/obsolete_pref_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_registry.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "brave/components/debounce/core/browser/debounce_service.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "brave/components/l10n/common/prefs.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/p3a/buildflags.h"
#include "brave/components/p3a/p3a_service.h"
#include "brave/components/p3a/star_randomness_meta.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/ios/browser/brave_stats/brave_stats_prefs.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "ios/chrome/browser/shared/model/prefs/pref_names.h"

void BraveRegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  brave_ads::RegisterProfilePrefs(registry);
  brave_ads::RegisterProfilePrefsForMigration(registry);
  brave_rewards::RegisterProfilePrefs(registry);
  brave_rewards::RegisterProfilePrefsForMigration(registry);
  brave_sync::Prefs::RegisterProfilePrefs(registry);
  brave_wallet::RegisterProfilePrefs(registry);
  brave_wallet::RegisterProfilePrefsForMigration(registry);
  de_amp::RegisterProfilePrefs(registry);
  debounce::DebounceService::RegisterProfilePrefs(registry);
  ai_chat::prefs::RegisterProfilePrefs(registry);
  ai_chat::ModelService::RegisterProfilePrefs(registry);
  omnibox::RegisterBraveProfilePrefs(registry);
  brave_news::prefs::RegisterProfilePrefs(registry);

  registry->RegisterBooleanPref(prefs::kHttpsUpgradesEnabled, true);
}

void BraveRegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  brave_stats::RegisterLocalStatePrefs(registry);
  brave_wallet::RegisterLocalStatePrefs(registry);
  brave_wallet::RegisterLocalStatePrefsForMigration(registry);
  decentralized_dns::RegisterLocalStatePrefs(registry);
  skus::RegisterLocalStatePrefs(registry);
#if BUILDFLAG(BRAVE_P3A_ENABLED)
  p3a::P3AService::RegisterPrefs(registry, false);
  p3a::StarRandomnessMeta::RegisterPrefsForMigration(registry);
#endif
  ntp_background_images::NTPBackgroundImagesService::RegisterLocalStatePrefs(
      registry);
  brave_l10n::RegisterL10nLocalStatePrefs(registry);
  ai_chat::prefs::RegisterLocalStatePrefs(registry);
  ai_chat::AIChatMetrics::RegisterPrefs(registry);

  // brave_shields
  // Note this can be removed when we use the entire
  // `brave_shields:AdBlockService` And we can call its
  // `RegisterPrefsForAdBlockService`
  registry->RegisterDictionaryPref(
      brave_shields::prefs::kAdBlockRegionalFilters);
  registry->RegisterDictionaryPref(
      brave_shields::prefs::kAdBlockListSubscriptions);
  registry->RegisterBooleanPref(
      brave_shields::prefs::kAdBlockCheckedAllDefaultRegions, false);
  registry->RegisterBooleanPref(
      brave_shields::prefs::kAdBlockCheckedDefaultRegion, false);
}

#define BRAVE_REGISTER_BROWSER_STATE_PREFS \
  BraveRegisterBrowserStatePrefs(registry);

#define BRAVE_REGISTER_LOCAL_STATE_PREFS BraveRegisterLocalStatePrefs(registry);

#define MigrateObsoleteProfilePrefs MigrateObsoleteProfilePrefs_ChromiumImpl

#define MigrateObsoleteLocalStatePrefs \
  MigrateObsoleteLocalStatePrefs_ChromiumImpl

#include "src/ios/chrome/browser/shared/model/prefs/browser_prefs.mm"

#undef MigrateObsoleteLocalStatePrefs
#undef MigrateObsoleteProfilePrefs
#undef BRAVE_REGISTER_LOCAL_STATE_PREFS
#undef BRAVE_REGISTER_BROWSER_STATE_PREFS

void MigrateObsoleteProfilePrefs(const base::FilePath& state_path,
                                 PrefService* prefs) {
  MigrateObsoleteProfilePrefs_ChromiumImpl(state_path, prefs);

  brave_ads::MigrateObsoleteProfilePrefs(prefs);
  brave_wallet::MigrateObsoleteProfilePrefs(prefs);
}

void MigrateObsoleteLocalStatePrefs(PrefService* prefs) {
  MigrateObsoleteLocalStatePrefs_ChromiumImpl(prefs);

#if BUILDFLAG(BRAVE_P3A_ENABLED)
  p3a::StarRandomnessMeta::MigrateObsoleteLocalStatePrefs(prefs);
#endif
}

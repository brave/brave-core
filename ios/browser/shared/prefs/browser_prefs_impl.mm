// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_account/prefs.h"
#include "brave/components/brave_ads/core/public/prefs/obsolete_pref_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/core/pref_registry.h"
#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "brave/components/debounce/core/browser/debounce_service.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "brave/components/l10n/common/prefs.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/common/view_counter_pref_registry.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/p3a/metric_log_store.h"
#include "brave/components/p3a/p3a_service.h"
#include "brave/components/p3a/rotation_scheduler.h"
#include "brave/components/playlist/core/common/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/ios/browser/brave_stats/brave_stats_prefs.h"
#include "components/pref_registry/pref_registry_syncable.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

namespace brave {

void RegisterBrowserStatePrefs(user_prefs::PrefRegistrySyncable* registry) {
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
  brave_account::prefs::RegisterPrefs(registry);
  omnibox::RegisterBraveProfilePrefs(registry);
  brave_news::prefs::RegisterProfilePrefs(registry);
  ntp_background_images::RegisterProfilePrefs(registry);
  ntp_background_images::RegisterProfilePrefsForMigration(registry);
  brave_shields::RegisterShieldsP3AProfilePrefs(registry);
  brave_shields::RegisterShieldsP3AProfilePrefsForMigration(registry);

  registry->RegisterBooleanPref(kBraveTalkDisabledByPolicy, false);

  // This is typically registered by the PlaylistService but iOS does not
  // use that service
  registry->RegisterBooleanPref(playlist::kPlaylistEnabledPref, true);

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  registry->RegisterBooleanPref(brave_vpn::prefs::kManagedBraveVPNDisabled,
                                false);
#endif
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  brave_stats::RegisterLocalStatePrefs(registry);
  brave_wallet::RegisterLocalStatePrefs(registry);
  brave_wallet::RegisterLocalStatePrefsForMigration(registry);
  decentralized_dns::RegisterLocalStatePrefs(registry);
  skus::RegisterLocalStatePrefs(registry);
  p3a::P3AService::RegisterPrefs(registry, false);
  p3a::MetricLogStore::RegisterLocalStatePrefsForMigration(registry);
  p3a::RotationScheduler::RegisterLocalStatePrefsForMigration(registry);
  ntp_background_images::NTPBackgroundImagesService::RegisterLocalStatePrefs(
      registry);
  brave_l10n::RegisterLocalStatePrefsForMigration(registry);
  ai_chat::prefs::RegisterLocalStatePrefs(registry);
  ai_chat::AIChatMetrics::RegisterPrefs(registry);
  ntp_background_images::RegisterLocalStatePrefs(registry);
  brave_shields::RegisterShieldsP3ALocalPrefs(registry);

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

void MigrateObsoleteProfilePrefs(PrefService* prefs) {
  brave_ads::MigrateObsoleteProfilePrefs(prefs);
  brave_wallet::MigrateObsoleteProfilePrefs(prefs);
  ntp_background_images::MigrateObsoleteProfilePrefs(prefs);
}

void MigrateObsoleteLocalStatePrefs(PrefService* prefs) {
  p3a::MetricLogStore::MigrateObsoleteLocalStatePrefs(prefs);
  p3a::RotationScheduler::MigrateObsoleteLocalStatePrefs(prefs);
}

}  // namespace brave

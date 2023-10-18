/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/common/pref_registry.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/l10n/common/prefs.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/p3a/buildflags.h"
#include "brave/components/p3a/p3a_service.h"
#include "brave/ios/browser/brave_stats/brave_stats_prefs.h"
#include "components/pref_registry/pref_registry_syncable.h"

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/ipfs/ipfs_service.h"
#endif

void BraveRegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  brave_rewards::RegisterProfilePrefs(registry);
  brave_rewards::RegisterProfilePrefsForMigration(registry);
  brave_sync::Prefs::RegisterProfilePrefs(registry);
  brave_wallet::RegisterProfilePrefs(registry);
  brave_wallet::RegisterProfilePrefsForMigration(registry);
#if BUILDFLAG(ENABLE_IPFS)
  ipfs::IpfsService::RegisterProfilePrefs(registry);
#endif
}

void BraveRegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  brave_stats::RegisterLocalStatePrefs(registry);
  brave_wallet::RegisterLocalStatePrefs(registry);
  brave_wallet::RegisterLocalStatePrefsForMigration(registry);
  decentralized_dns::RegisterLocalStatePrefs(registry);
#if BUILDFLAG(BRAVE_P3A_ENABLED)
  p3a::P3AService::RegisterPrefs(registry, false);
#endif
  ntp_background_images::NTPBackgroundImagesService::RegisterLocalStatePrefs(
      registry);
  brave_l10n::RegisterL10nLocalStatePrefs(registry);
}

void BraveMigrateObsoleteBrowserStatePrefs(PrefService* prefs) {
  brave_wallet::KeyringService::MigrateObsoleteProfilePrefs(prefs);
  brave_wallet::MigrateObsoleteProfilePrefs(prefs);
}

#define BRAVE_REGISTER_BROWSER_STATE_PREFS \
  BraveRegisterBrowserStatePrefs(registry);

#define BRAVE_REGISTER_LOCAL_STATE_PREFS BraveRegisterLocalStatePrefs(registry);

#define BRAVE_MIGRATE_OBSOLETE_BROWSER_STATE_PREFS \
  BraveMigrateObsoleteBrowserStatePrefs(prefs);

#include "src/ios/chrome/browser/shared/model/prefs/browser_prefs.mm"
#undef BRAVE_MIGRATE_OBSOLETE_BROWSER_STATE_PREFS
#undef BRAVE_REGISTER_LOCAL_STATE_PREFS
#undef BRAVE_REGISTER_BROWSER_STATE_PREFS

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_stats/brave_stats_prefs.h"

#include "base/time/time.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/components/brave_wallet/browser/pref_names.h"
#endif

namespace brave_stats {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kStatsReportingEnabled, true);
}

void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry) {
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  // Deprecated 12/2025
  registry->RegisterTimePref(
      brave_wallet::kBraveWalletPingReportedUnlockTimeDeprecated, base::Time());
#endif
}

void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  // Deprecated 12/2025
  local_state->ClearPref(
      brave_wallet::kBraveWalletPingReportedUnlockTimeDeprecated);
#endif
}

}  // namespace brave_stats

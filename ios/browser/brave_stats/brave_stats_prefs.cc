/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_stats/brave_stats_prefs.h"

#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace brave_stats {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kBraveWalletPingReportedUnlockTime, base::Time());
}

}  // namespace brave_stats

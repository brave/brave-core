/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_local_state_prefs.h"

#include "base/values.h"
#include "brave/browser/brave_stats_updater.h"
#include "brave/browser/metrics/metrics_reporting_util.h"
#include "brave/browser/tor/tor_profile_service.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/common/pref_names.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace brave {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  brave_shields::RegisterPrefsForAdBlockService(registry);
  RegisterPrefsForBraveStatsUpdater(registry);
#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  RegisterPrefsForBraveReferralsService(registry);
#endif
#if defined(OS_MACOSX)
  // Turn off super annoying 'Hold to quit'
  registry->SetDefaultPrefValue(prefs::kConfirmToQuitEnabled,
      base::Value(false));
#endif
  tor::TorProfileService::RegisterLocalStatePrefs(registry);
#if !defined(OS_ANDROID)
  RegisterPrefsForMuonMigration(registry);
#endif

  registry->SetDefaultPrefValue(
      metrics::prefs::kMetricsReportingEnabled,
      base::Value(GetDefaultPrefValueForMetricsReporting()));
}

}  // namespace brave

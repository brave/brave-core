/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_local_state_prefs.h"

#include "base/values.h"
#include "brave/browser/brave_stats_updater.h"
#include "brave/browser/metrics/metrics_reporting_util.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/brave_shields_p3a.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_utils.h"
#include "brave/components/p3a/brave_p3a_service.h"
#include "brave/components/p3a/buildflags.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/common/pref_names.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service.h"
#endif

#if !defined(OS_ANDROID)
#include "brave/browser/p3a/p3a_core_metrics.h"
#endif  // !defined(OS_ANDROID)

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/widevine/widevine_utils.h"
#endif

namespace brave {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  brave_shields::RegisterPrefsForAdBlockService(registry);
  RegisterPrefsForBraveStatsUpdater(registry);
  registry->RegisterBooleanPref(kRemoteDebuggingEnabled, false);
  ntp_background_images::RegisterLocalStatePrefs(registry);
#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  RegisterPrefsForBraveReferralsService(registry);
#endif
#if defined(OS_MACOSX)
  // Turn off super annoying 'Hold to quit'
  registry->SetDefaultPrefValue(prefs::kConfirmToQuitEnabled,
      base::Value(false));
#endif
#if BUILDFLAG(ENABLE_TOR)
  tor::TorProfileService::RegisterLocalStatePrefs(registry);
#endif
  registry->SetDefaultPrefValue(
      metrics::prefs::kMetricsReportingEnabled,
      base::Value(GetDefaultPrefValueForMetricsReporting()));

#if BUILDFLAG(BRAVE_P3A_ENABLED)
  brave::BraveP3AService::RegisterPrefs(registry,
                                        first_run::IsChromeFirstRun());
#endif  // BUILDFLAG(BRAVE_P3A_ENABLED)

  brave_shields::RegisterShieldsP3APrefs(registry);
#if !defined(OS_ANDROID)
  BraveWindowTracker::RegisterPrefs(registry);
  BraveUptimeTracker::RegisterPrefs(registry);
  dark_mode::RegisterBraveDarkModeLocalStatePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
  RegisterWidevineLocalstatePrefs(registry);
#endif
}

}  // namespace brave

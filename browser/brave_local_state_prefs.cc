/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_local_state_prefs.h"

#include "base/values.h"
#include "brave/browser/brave_stats/brave_stats_updater.h"
#include "brave/browser/metrics/metrics_reporting_util.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_federated_learning/brave_federated_learning_service.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/brave_shields_p3a.h"
#include "brave/components/decentralized_dns/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "brave/components/p3a/brave_p3a_service.h"
#include "brave/components/p3a/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/common/pref_names.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/tor_profile_service.h"
#endif

#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_message_handler.h"

#if !defined(OS_ANDROID)
#include "brave/browser/p3a/p3a_core_metrics.h"
#include "chrome/browser/first_run/first_run.h"
#endif  // !defined(OS_ANDROID)

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/widevine/widevine_utils.h"
#endif

#if BUILDFLAG(DECENTRALIZED_DNS_ENABLED)
#include "brave/components/decentralized_dns/decentralized_dns_service.h"
#endif

namespace brave {

void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry) {
#if BUILDFLAG(ENABLE_WIDEVINE)
  RegisterWidevineLocalstatePrefsForMigration(registry);
#endif
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  brave_shields::RegisterPrefsForAdBlockService(registry);
  brave_stats::RegisterLocalStatePrefs(registry);
  ntp_background_images::NTPBackgroundImagesService::RegisterLocalStatePrefs(
      registry);
  ntp_background_images::ViewCounterService::RegisterLocalStatePrefs(registry);
  brave::BraveFederatedLearningService::RegisterLocalStatePrefs(registry);
#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
  RegisterPrefsForBraveReferralsService(registry);
#endif
#if defined(OS_MAC)
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
#if !defined(OS_ANDROID)
                                        first_run::IsChromeFirstRun());
#else
                                        // BraveP3AService::RegisterPrefs
                                        // doesn't use this arg on Android
                                        false);
#endif  // !defined(OS_ANDROID)

#endif  // BUILDFLAG(BRAVE_P3A_ENABLED)

  brave_shields::RegisterShieldsP3APrefs(registry);
#if !defined(OS_ANDROID)
  BraveNewTabMessageHandler::RegisterLocalStatePrefs(registry);
  BraveWindowTracker::RegisterPrefs(registry);
  BraveUptimeTracker::RegisterPrefs(registry);
  dark_mode::RegisterBraveDarkModeLocalStatePrefs(registry);

  registry->RegisterBooleanPref(kDefaultBrowserPromptEnabled, true);
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
  RegisterWidevineLocalstatePrefs(registry);
#endif

#if BUILDFLAG(DECENTRALIZED_DNS_ENABLED)
  decentralized_dns::DecentralizedDnsService::RegisterLocalStatePrefs(registry);
#endif

  RegisterLocalStatePrefsForMigration(registry);
}

}  // namespace brave

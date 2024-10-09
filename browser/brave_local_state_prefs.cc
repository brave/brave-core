/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_local_state_prefs.h"

#include <string>

#include "base/values.h"
#include "brave/browser/brave_ads/analytics/p3a/brave_stats_helper.h"
#include "brave/browser/brave_stats/brave_stats_updater.h"
#include "brave/browser/metrics/buildflags/buildflags.h"
#include "brave/browser/metrics/metrics_reporting_util.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/misc_metrics/uptime_monitor.h"
#include "brave/browser/ntp_background/ntp_p3a_helper_impl.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/browser/search_engines/search_engine_tracker.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/brave_search_conversion/p3a.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_p3a.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "brave/components/l10n/common/prefs.h"
#include "brave/components/misc_metrics/general_browser_usage.h"
#include "brave/components/misc_metrics/page_metrics.h"
#include "brave/components/misc_metrics/privacy_hub_metrics.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "brave/components/p3a/p3a_service.h"
#include "brave/components/p3a/star_randomness_meta.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/common/pref_names.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_ANDROID)
#include "brave/browser/day_zero_browser_ui_expt/day_zero_browser_ui_expt_manager.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/tor_profile_service.h"
#endif

#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_message_handler.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/p3a/p3a_core_metrics.h"
#include "brave/browser/search_engines/pref_names.h"
#include "brave/browser/ui/whats_new/whats_new_util.h"
#include "chrome/browser/first_run/first_run.h"
#endif  // !BUILDFLAG(IS_ANDROID)

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/onboarding/onboarding_tab_helper.h"
#include "brave/components/sidebar/browser/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/widevine/widevine_utils.h"
#endif

namespace brave {

void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry) {
#if !BUILDFLAG(IS_ANDROID)
  // Added 10/2022
  registry->RegisterBooleanPref(kDefaultBrowserPromptEnabled, true);
#endif

  misc_metrics::UptimeMonitor::RegisterPrefsForMigration(registry);
  brave_wallet::RegisterLocalStatePrefsForMigration(registry);
  brave_search_conversion::p3a::RegisterLocalStatePrefsForMigration(registry);
  brave_stats::RegisterLocalStatePrefsForMigration(registry);
  p3a::StarRandomnessMeta::RegisterPrefsForMigration(registry);
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  brave_shields::RegisterPrefsForAdBlockService(registry);
  brave_stats::RegisterLocalStatePrefs(registry);
  ntp_background_images::NTPBackgroundImagesService::RegisterLocalStatePrefs(
      registry);
  ntp_background_images::ViewCounterService::RegisterLocalStatePrefs(registry);
  RegisterPrefsForBraveReferralsService(registry);
  brave_l10n::RegisterL10nLocalStatePrefs(registry);
#if BUILDFLAG(IS_MAC)
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

  p3a::P3AService::RegisterPrefs(registry,
#if !BUILDFLAG(IS_ANDROID)
                                 first_run::IsChromeFirstRun());
#else
                                 // BraveP3AService::RegisterPrefs
                                 // doesn't use this arg on Android
                                 false);
#endif  // !BUILDFLAG(IS_ANDROID)

  brave_shields::RegisterShieldsP3ALocalPrefs(registry);
#if !BUILDFLAG(IS_ANDROID)
  BraveNewTabMessageHandler::RegisterLocalStatePrefs(registry);
  BraveWindowTracker::RegisterPrefs(registry);
  dark_mode::RegisterBraveDarkModeLocalStatePrefs(registry);
  whats_new::RegisterLocalStatePrefs(registry);

  registry->RegisterBooleanPref(kEnableSearchSuggestionsByDefault, false);
#endif

#if defined(TOOLKIT_VIEWS)
  onboarding::RegisterLocalStatePrefs(registry);
  registry->RegisterBooleanPref(sidebar::kTargetUserForSidebarEnabledTest,
                                false);
#endif

#if BUILDFLAG(ENABLE_CRASH_DIALOG)
  registry->RegisterBooleanPref(kDontAskForCrashReporting, false);
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
  RegisterWidevineLocalstatePrefs(registry);
#endif

  decentralized_dns::RegisterLocalStatePrefs(registry);

  RegisterLocalStatePrefsForMigration(registry);

  brave_search_conversion::p3a::RegisterLocalStatePrefs(registry);

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn::RegisterLocalStatePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
  ai_chat::prefs::RegisterLocalStatePrefs(registry);
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN) || BUILDFLAG(ENABLE_AI_CHAT)
  skus::RegisterLocalStatePrefs(registry);
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_ANDROID)
  DayZeroBrowserUIExptManager::RegisterLocalStatePrefs(registry);
#endif

  registry->RegisterStringPref(::prefs::kBraveVpnDnsConfig, std::string());

  ntp_background_images::NTPP3AHelperImpl::RegisterLocalStatePrefs(registry);

  brave_wallet::RegisterLocalStatePrefs(registry);

  misc_metrics::ProcessMiscMetrics::RegisterPrefs(registry);
  misc_metrics::PageMetrics::RegisterPrefs(registry);
  brave_ads::BraveStatsHelper::RegisterLocalStatePrefs(registry);
  misc_metrics::GeneralBrowserUsage::RegisterPrefs(registry);

  playlist::PlaylistServiceFactory::RegisterLocalStatePrefs(registry);
}

}  // namespace brave

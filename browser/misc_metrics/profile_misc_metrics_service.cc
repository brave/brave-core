/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/profile_misc_metrics_service.h"

#include "brave/components/misc_metrics/autofill_metrics.h"
#include "brave/components/misc_metrics/language_metrics.h"
#include "brave/components/misc_metrics/page_metrics.h"
#include "chrome/browser/autofill/personal_data_manager_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/misc_android_metrics.h"
#include "brave/browser/search_engines/search_engine_tracker.h"
#else
#include "brave/browser/misc_metrics/extension_metrics.h"
#include "extensions/browser/extension_registry_factory.h"
#endif

namespace misc_metrics {

ProfileMiscMetricsService::ProfileMiscMetricsService(
    content::BrowserContext* context) {
  auto* profile_prefs = user_prefs::UserPrefs::Get(context);
  auto* local_state = g_browser_process->local_state();
  if (profile_prefs) {
    language_metrics_ = std::make_unique<LanguageMetrics>(profile_prefs);
  }
  auto* history_service = HistoryServiceFactory::GetForProfile(
      Profile::FromBrowserContext(context), ServiceAccessType::EXPLICIT_ACCESS);
  auto* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(context);
  if (history_service && host_content_settings_map) {
    page_metrics_ = std::make_unique<PageMetrics>(
        local_state, host_content_settings_map, history_service);
  }
#if BUILDFLAG(IS_ANDROID)
  auto* search_engine_tracker =
      SearchEngineTrackerFactory::GetInstance()->GetForBrowserContext(context);
  misc_android_metrics_ = std::make_unique<MiscAndroidMetrics>(
      g_brave_browser_process->process_misc_metrics(), search_engine_tracker);
#else
  extensions::ExtensionRegistry* extension_registry =
      extensions::ExtensionRegistryFactory::GetForBrowserContext(context);
  if (extension_registry) {
    extension_metrics_ = std::make_unique<ExtensionMetrics>(extension_registry);
  }
#endif
  auto* personal_data_manager =
      autofill::PersonalDataManagerFactory::GetInstance()->GetForBrowserContext(
          context);
  if (personal_data_manager) {
    autofill_metrics_ =
        std::make_unique<AutofillMetrics>(personal_data_manager);
  }
}

ProfileMiscMetricsService::~ProfileMiscMetricsService() = default;

void ProfileMiscMetricsService::Shutdown() {
#if !BUILDFLAG(IS_ANDROID)
  if (extension_metrics_) {
    extension_metrics_->Shutdown();
  }
#endif
}

PageMetrics* ProfileMiscMetricsService::GetPageMetrics() {
  return page_metrics_.get();
}

#if BUILDFLAG(IS_ANDROID)
MiscAndroidMetrics* ProfileMiscMetricsService::GetMiscAndroidMetrics() {
  return misc_android_metrics_.get();
}
#endif

}  // namespace misc_metrics

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/profile_misc_metrics_service_factory.h"

#include "base/no_destructor.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service.h"
#include "chrome/browser/autofill/personal_data_manager_factory.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/themes/theme_service_factory.h"
#include "extensions/browser/extension_registry_factory.h"
#else
#include "brave/browser/search_engines/search_engine_tracker.h"
#endif

namespace misc_metrics {

// static
ProfileMiscMetricsServiceFactory*
ProfileMiscMetricsServiceFactory::GetInstance() {
  static base::NoDestructor<ProfileMiscMetricsServiceFactory> instance;
  return instance.get();
}

// static
ProfileMiscMetricsService*
ProfileMiscMetricsServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  return static_cast<ProfileMiscMetricsService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

ProfileMiscMetricsServiceFactory::ProfileMiscMetricsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "ProfileMiscMetricsService",
          BrowserContextDependencyManager::GetInstance()) {
#if !BUILDFLAG(IS_ANDROID)
  DependsOn(extensions::ExtensionRegistryFactory::GetInstance());
  DependsOn(ThemeServiceFactory::GetInstance());
#else
  DependsOn(SearchEngineTrackerFactory::GetInstance());
#endif
  DependsOn(HistoryServiceFactory::GetInstance());
  DependsOn(HostContentSettingsMapFactory::GetInstance());
  DependsOn(autofill::PersonalDataManagerFactory::GetInstance());
  DependsOn(BookmarkModelFactory::GetInstance());
}

ProfileMiscMetricsServiceFactory::~ProfileMiscMetricsServiceFactory() = default;

KeyedService* ProfileMiscMetricsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new ProfileMiscMetricsService(context);
}

content::BrowserContext*
ProfileMiscMetricsServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord()) {
    return nullptr;
  }
  return context;
}

}  // namespace misc_metrics

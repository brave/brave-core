/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/profile_misc_metrics_service_factory.h"

#include "base/no_destructor.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service.h"
#include "brave/browser/search_engines/search_engine_tracker.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/extension_registry_factory.h"

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
  DependsOn(extensions::ExtensionRegistryFactory::GetInstance());
  DependsOn(HistoryServiceFactory::GetInstance());
  DependsOn(SearchEngineTrackerFactory::GetInstance());
}

ProfileMiscMetricsServiceFactory::~ProfileMiscMetricsServiceFactory() = default;

KeyedService* ProfileMiscMetricsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* extension_registry =
      extensions::ExtensionRegistryFactory::GetForBrowserContext(context);
  auto* history_service = HistoryServiceFactory::GetForProfile(
      Profile::FromBrowserContext(context), ServiceAccessType::EXPLICIT_ACCESS);
  auto* search_engine_tracker =
      SearchEngineTrackerFactory::GetInstance()->GetForBrowserContext(context);
  return new ProfileMiscMetricsService(extension_registry, history_service,
                                       search_engine_tracker);
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

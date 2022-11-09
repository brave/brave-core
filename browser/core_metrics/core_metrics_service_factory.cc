/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/core_metrics/core_metrics_service_factory.h"
#include "brave/components/core_metrics/core_metrics_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace core_metrics {

// static
CoreMetricsServiceFactory* CoreMetricsServiceFactory::GetInstance() {
  return base::Singleton<CoreMetricsServiceFactory>::get();
}

// static
CoreMetricsService* CoreMetricsServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (context->IsOffTheRecord()) {
    return nullptr;
  }
  return static_cast<CoreMetricsService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

CoreMetricsServiceFactory::CoreMetricsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "CoreMetricsService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(HistoryServiceFactory::GetInstance());
}

CoreMetricsServiceFactory::~CoreMetricsServiceFactory() = default;

KeyedService* CoreMetricsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  history::HistoryService* history_service =
      HistoryServiceFactory::GetForProfile(Profile::FromBrowserContext(context),
                                           ServiceAccessType::EXPLICIT_ACCESS);
  return new CoreMetricsService(g_browser_process->local_state(),
                                history_service);
}

}  // namespace core_metrics

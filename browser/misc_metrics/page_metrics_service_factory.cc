/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/page_metrics_service_factory.h"

#include "base/no_destructor.h"
#include "brave/components/misc_metrics/page_metrics_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace misc_metrics {

// static
PageMetricsServiceFactory* PageMetricsServiceFactory::GetInstance() {
  static base::NoDestructor<PageMetricsServiceFactory> instance;
  return instance.get();
}

// static
PageMetricsService* PageMetricsServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (context->IsOffTheRecord()) {
    return nullptr;
  }
  return static_cast<PageMetricsService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

PageMetricsServiceFactory::PageMetricsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "PageMetricsService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(HistoryServiceFactory::GetInstance());
}

PageMetricsServiceFactory::~PageMetricsServiceFactory() = default;

KeyedService* PageMetricsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  history::HistoryService* history_service =
      HistoryServiceFactory::GetForProfile(Profile::FromBrowserContext(context),
                                           ServiceAccessType::EXPLICIT_ACCESS);
  return new PageMetricsService(g_browser_process->local_state(),
                                history_service);
}

}  // namespace misc_metrics

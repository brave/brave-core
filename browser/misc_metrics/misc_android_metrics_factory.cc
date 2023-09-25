/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/misc_android_metrics_factory.h"

#include "base/no_destructor.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/misc_android_metrics.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace misc_metrics {

class MiscAndroidMetrics;

MiscAndroidMetricsFactory* MiscAndroidMetricsFactory::GetInstance() {
  static base::NoDestructor<MiscAndroidMetricsFactory> instance;
  return instance.get();
}

MiscAndroidMetricsFactory::MiscAndroidMetricsFactory()
    : BrowserContextKeyedServiceFactory(
          "MiscAndroidMetrics",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(SearchEngineTrackerFactory::GetInstance());
}

MiscAndroidMetricsFactory::~MiscAndroidMetricsFactory() = default;

MiscAndroidMetrics* MiscAndroidMetricsFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<MiscAndroidMetrics*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

KeyedService* MiscAndroidMetricsFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new MiscAndroidMetrics(
      g_brave_browser_process->process_misc_metrics(),
      SearchEngineTrackerFactory::GetInstance()->GetForBrowserContext(context));
}

}  // namespace misc_metrics

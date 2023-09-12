/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_MISC_ANDROID_METRICS_FACTORY_H_
#define BRAVE_BROWSER_MISC_METRICS_MISC_ANDROID_METRICS_FACTORY_H_

#include "base/memory/raw_ptr.h"
#include "brave/browser/search_engines/search_engine_tracker.h"
#include "brave/components/misc_metrics/common/misc_metrics.mojom.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace misc_metrics {

class MiscAndroidMetrics;

class MiscAndroidMetricsFactory : public BrowserContextKeyedServiceFactory {
 public:
  static MiscAndroidMetricsFactory* GetInstance();
  static MiscAndroidMetrics* GetForBrowserContext(
      content::BrowserContext* context);

  MiscAndroidMetricsFactory(const MiscAndroidMetricsFactory&) = delete;
  MiscAndroidMetricsFactory& operator=(const MiscAndroidMetricsFactory&) =
      delete;

 private:
  friend base::NoDestructor<MiscAndroidMetricsFactory>;
  MiscAndroidMetricsFactory();
  ~MiscAndroidMetricsFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_MISC_ANDROID_METRICS_FACTORY_H_

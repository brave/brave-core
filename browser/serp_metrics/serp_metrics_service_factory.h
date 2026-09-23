/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_SERVICE_FACTORY_H_

#include <memory>

#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace serp_metrics {

class SerpMetricsService;

// A factory that creates `SerpMetricsService` which is used to store SERP
// metrics for a profile.
class SerpMetricsServiceFactory : public ProfileKeyedServiceFactory {
 public:
  SerpMetricsServiceFactory(const SerpMetricsServiceFactory&) = delete;
  SerpMetricsServiceFactory& operator=(const SerpMetricsServiceFactory&) =
      delete;

  static SerpMetricsServiceFactory* GetInstance();
  static SerpMetricsService* GetFor(content::BrowserContext* context);

 private:
  friend base::NoDestructor<SerpMetricsServiceFactory>;

  SerpMetricsServiceFactory();
  ~SerpMetricsServiceFactory() override;

  // ProfileKeyedServiceFactory:
  bool ServiceIsNULLWhileTesting() const override;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace serp_metrics

#endif  // BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_SERVICE_FACTORY_H_

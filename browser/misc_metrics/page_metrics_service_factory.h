/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_PAGE_METRICS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_MISC_METRICS_PAGE_METRICS_SERVICE_FACTORY_H_

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace misc_metrics {

class PageMetricsService;

class PageMetricsServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static PageMetricsService* GetServiceForContext(
      content::BrowserContext* context);
  static PageMetricsServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<PageMetricsServiceFactory>;

  PageMetricsServiceFactory();
  ~PageMetricsServiceFactory() override;

  PageMetricsServiceFactory(const PageMetricsServiceFactory&) = delete;
  PageMetricsServiceFactory& operator=(const PageMetricsServiceFactory&) =
      delete;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_PAGE_METRICS_SERVICE_FACTORY_H_

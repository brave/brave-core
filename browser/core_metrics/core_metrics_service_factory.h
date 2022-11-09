/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_CORE_METRICS_CORE_METRICS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_CORE_METRICS_CORE_METRICS_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"

namespace core_metrics {

class CoreMetricsService;

class CoreMetricsServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static CoreMetricsService* GetServiceForContext(
      content::BrowserContext* context);
  static CoreMetricsServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<CoreMetricsServiceFactory>;

  CoreMetricsServiceFactory();
  ~CoreMetricsServiceFactory() override;

  CoreMetricsServiceFactory(const CoreMetricsServiceFactory&) = delete;
  CoreMetricsServiceFactory& operator=(const CoreMetricsServiceFactory&) =
      delete;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

}  // namespace core_metrics

#endif  // BRAVE_BROWSER_CORE_METRICS_CORE_METRICS_SERVICE_FACTORY_H_

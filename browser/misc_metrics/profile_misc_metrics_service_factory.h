/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_PROFILE_MISC_METRICS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_MISC_METRICS_PROFILE_MISC_METRICS_SERVICE_FACTORY_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace misc_metrics {

class ProfileMiscMetricsService;

class ProfileMiscMetricsServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static ProfileMiscMetricsService* GetServiceForContext(
      content::BrowserContext* context);
  static ProfileMiscMetricsServiceFactory* GetInstance();

  ProfileMiscMetricsServiceFactory(const ProfileMiscMetricsServiceFactory&) =
      delete;
  ProfileMiscMetricsServiceFactory& operator=(
      const ProfileMiscMetricsServiceFactory&) = delete;

 private:
  friend base::NoDestructor<ProfileMiscMetricsServiceFactory>;

  ProfileMiscMetricsServiceFactory();
  ~ProfileMiscMetricsServiceFactory() override;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_PROFILE_MISC_METRICS_SERVICE_FACTORY_H_

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_SERVICE_FACTORY_IOS_H_
#define BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_SERVICE_FACTORY_IOS_H_

#include <memory>

#include "ios/chrome/browser/shared/model/profile/profile_keyed_service_factory_ios.h"

class ProfileIOS;
class KeyedService;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace serp_metrics {

class SerpMetrics;

// A factory that creates `SerpMetricsServiceIOS` for non-incognito profile.
class SerpMetricsServiceFactoryIOS : public ProfileKeyedServiceFactoryIOS {
 public:
  SerpMetricsServiceFactoryIOS(const SerpMetricsServiceFactoryIOS&) = delete;
  SerpMetricsServiceFactoryIOS& operator=(const SerpMetricsServiceFactoryIOS&) =
      delete;

  static SerpMetricsServiceFactoryIOS* GetInstance();

  // Returns the `SerpMetrics` for `profile`, or nullptr if the feature is
  // disabled or `profile` is off-the-record.
  static SerpMetrics* GetForProfile(ProfileIOS* profile);

 private:
  friend base::NoDestructor<SerpMetricsServiceFactoryIOS>;

  SerpMetricsServiceFactoryIOS();
  ~SerpMetricsServiceFactoryIOS() override;

  // ProfileKeyedServiceFactoryIOS:
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;
};

}  // namespace serp_metrics

#endif  // BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_SERVICE_FACTORY_IOS_H_

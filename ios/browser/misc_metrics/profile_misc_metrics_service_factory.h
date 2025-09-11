/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_MISC_METRICS_PROFILE_MISC_METRICS_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_MISC_METRICS_PROFILE_MISC_METRICS_SERVICE_FACTORY_H_

#include <memory>

#include "components/keyed_service/core/keyed_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_keyed_service_factory_ios.h"

class ProfileIOS;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace misc_metrics {

class ProfileMiscMetricsService;

class ProfileMiscMetricsServiceFactory : public ProfileKeyedServiceFactoryIOS {
 public:
  static ProfileMiscMetricsService* GetForProfile(ProfileIOS* profile);
  static ProfileMiscMetricsServiceFactory* GetInstance();

  ProfileMiscMetricsServiceFactory(const ProfileMiscMetricsServiceFactory&) =
      delete;
  ProfileMiscMetricsServiceFactory& operator=(
      const ProfileMiscMetricsServiceFactory&) = delete;

 private:
  friend base::NoDestructor<ProfileMiscMetricsServiceFactory>;

  ProfileMiscMetricsServiceFactory();
  ~ProfileMiscMetricsServiceFactory() override;

  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;
};

}  // namespace misc_metrics

#endif  // BRAVE_IOS_BROWSER_MISC_METRICS_PROFILE_MISC_METRICS_SERVICE_FACTORY_H_

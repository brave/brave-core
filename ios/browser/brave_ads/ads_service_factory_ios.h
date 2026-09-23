/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_SERVICE_FACTORY_IOS_H_
#define BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_SERVICE_FACTORY_IOS_H_

#include <memory>

#include "ios/chrome/browser/shared/model/profile/profile_keyed_service_factory_ios.h"

class ProfileIOS;
class KeyedService;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave_ads {

class AdsServiceImplIOS;

class AdsServiceFactoryIOS : public ProfileKeyedServiceFactoryIOS {
 public:
  AdsServiceFactoryIOS(const AdsServiceFactoryIOS&) = delete;
  AdsServiceFactoryIOS& operator=(const AdsServiceFactoryIOS&) = delete;

  AdsServiceFactoryIOS(AdsServiceFactoryIOS&&) = delete;
  AdsServiceFactoryIOS& operator=(AdsServiceFactoryIOS&&) = delete;

  static AdsServiceImplIOS* GetForProfile(ProfileIOS* profile);

  static AdsServiceFactoryIOS* GetInstance();

 private:
  friend base::NoDestructor<AdsServiceFactoryIOS>;

  AdsServiceFactoryIOS();
  ~AdsServiceFactoryIOS() override;

  // ProfileKeyedServiceFactoryIOS:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;
};

}  // namespace brave_ads

#endif  // BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_SERVICE_FACTORY_IOS_H_

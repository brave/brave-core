/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_SERVICE_FACTORY_IOS_H_
#define BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_SERVICE_FACTORY_IOS_H_

#include <memory>

#include "brave/ios/browser/brave_ads/ads_service_impl_ios.h"
#include "ios/chrome/browser/shared/model/profile/typed_profile_keyed_service_factory_ios.h"

class KeyedService;
class ProfileIOS;

namespace brave_ads {

class AdsServiceFactoryIOS
    : public TypedProfileKeyedServiceFactoryIOS<AdsServiceFactoryIOS,
                                                AdsServiceImplIOS> {
 public:
  explicit AdsServiceFactoryIOS(PassKey key);

  AdsServiceFactoryIOS(const AdsServiceFactoryIOS&) = delete;
  AdsServiceFactoryIOS& operator=(const AdsServiceFactoryIOS&) = delete;

  AdsServiceFactoryIOS(AdsServiceFactoryIOS&&) = delete;
  AdsServiceFactoryIOS& operator=(AdsServiceFactoryIOS&&) = delete;

 private:
  // TypedProfileKeyedServiceFactoryIOS:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;
};

}  // namespace brave_ads

#endif  // BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_SERVICE_FACTORY_IOS_H_

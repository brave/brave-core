/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_ADS_ADS_SERVICE_FACTORY_IOS_H_
#define BRAVE_IOS_BROWSER_ADS_ADS_SERVICE_FACTORY_IOS_H_

#include <memory>

#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

class ProfileIOS;
class KeyedService;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace web {
class BrowserState;
}  // namespace web

namespace brave_ads {

class AdsServiceImplIOS;

class AdsServiceFactoryIOS : public BrowserStateKeyedServiceFactory {
 public:
  AdsServiceFactoryIOS(const AdsServiceFactoryIOS&) = delete;
  AdsServiceFactoryIOS& operator=(const AdsServiceFactoryIOS&) = delete;

  AdsServiceFactoryIOS(AdsServiceFactoryIOS&&) = delete;
  AdsServiceFactoryIOS& operator=(AdsServiceFactoryIOS&&) = delete;

  static AdsServiceImplIOS* GetForBrowserState(ProfileIOS* profile);

  static AdsServiceFactoryIOS* GetInstance();

 private:
  friend base::NoDestructor<AdsServiceFactoryIOS>;

  AdsServiceFactoryIOS();
  ~AdsServiceFactoryIOS() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
};

}  // namespace brave_ads

#endif  // BRAVE_IOS_BROWSER_ADS_ADS_SERVICE_FACTORY_IOS_H_

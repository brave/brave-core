// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_DEBOUNCE_DEBOUNCE_SERVICE_FACTORY_PRIVATE_H_
#define BRAVE_IOS_BROWSER_DEBOUNCE_DEBOUNCE_SERVICE_FACTORY_PRIVATE_H_

#include <memory>

#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper.h"
#include "ios/chrome/browser/shared/model/profile/profile_keyed_service_factory_ios.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

class KeyedService;
class ProfileIOS;

namespace debounce {
class DebounceService;

class DebounceServiceFactory : public ProfileKeyedServiceFactoryIOS {
 public:
  static debounce::DebounceService* GetServiceForState(ProfileIOS* profile);

  static DebounceServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<DebounceServiceFactory>;

  DebounceServiceFactory();
  ~DebounceServiceFactory() override;

  // ProfileKeyedServiceFactoryIOS implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;
};

}  // namespace debounce

#endif  // BRAVE_IOS_BROWSER_DEBOUNCE_DEBOUNCE_SERVICE_FACTORY_PRIVATE_H_

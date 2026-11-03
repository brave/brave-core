// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_AD_BLOCK_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_AD_BLOCK_SERVICE_FACTORY_H_

#include <memory>

#include "ios/chrome/browser/shared/model/profile/profile_keyed_service_factory_ios.h"

class ProfileIOS;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave_shields {

class AdBlockService;

// Returns nullptr unless `kBraveIOSSharedAdBlockService` is enabled.
class AdBlockServiceFactory : public ProfileKeyedServiceFactoryIOS {
 public:
  static AdBlockService* GetForProfile(ProfileIOS* profile);
  static AdBlockService* GetForProfileIfExists(ProfileIOS* profile);
  static AdBlockServiceFactory* GetInstance();

  AdBlockServiceFactory(const AdBlockServiceFactory&) = delete;
  AdBlockServiceFactory& operator=(const AdBlockServiceFactory&) = delete;

 private:
  friend class base::NoDestructor<AdBlockServiceFactory>;

  AdBlockServiceFactory();
  ~AdBlockServiceFactory() override;

  // ProfileKeyedServiceFactoryIOS:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;
};

}  // namespace brave_shields

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_AD_BLOCK_SERVICE_FACTORY_H_

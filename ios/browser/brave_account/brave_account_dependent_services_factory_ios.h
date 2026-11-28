/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DEPENDENT_SERVICES_FACTORY_IOS_H_
#define BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DEPENDENT_SERVICES_FACTORY_IOS_H_

#include <memory>

#include "brave/components/brave_account/brave_account_dependent_services.h"
#include "ios/chrome/browser/shared/model/profile/typed_profile_keyed_service_factory_ios.h"

class KeyedService;
class ProfileIOS;

namespace brave_account {

class BraveAccountDependentServicesFactoryIOS
    : public TypedProfileKeyedServiceFactoryIOS<
          BraveAccountDependentServicesFactoryIOS,
          BraveAccountDependentServices> {
 public:
  static BraveAccountDependentServices* GetFor(ProfileIOS* profile);

  explicit BraveAccountDependentServicesFactoryIOS(PassKey key);

  BraveAccountDependentServicesFactoryIOS(
      const BraveAccountDependentServicesFactoryIOS&) = delete;
  BraveAccountDependentServicesFactoryIOS& operator=(
      const BraveAccountDependentServicesFactoryIOS&) = delete;

 private:
  // TypedProfileKeyedServiceFactoryIOS:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS& profile) const override;
};

}  // namespace brave_account

#endif  // BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DEPENDENT_SERVICES_FACTORY_IOS_H_

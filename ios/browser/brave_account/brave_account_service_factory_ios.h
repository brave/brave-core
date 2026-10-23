/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_FACTORY_IOS_H_
#define BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_FACTORY_IOS_H_

#include <memory>

#include "brave/components/brave_account/brave_account_service.h"
#include "ios/chrome/browser/shared/model/profile/typed_profile_keyed_service_factory_ios.h"

class KeyedService;
class ProfileIOS;

namespace brave_account {

class BraveAccountServiceFactoryIOS
    : public TypedProfileKeyedServiceFactoryIOS<BraveAccountServiceFactoryIOS,
                                                BraveAccountService> {
 public:
  explicit BraveAccountServiceFactoryIOS(PassKey key);

  BraveAccountServiceFactoryIOS(const BraveAccountServiceFactoryIOS&) = delete;
  BraveAccountServiceFactoryIOS& operator=(
      const BraveAccountServiceFactoryIOS&) = delete;

  static BraveAccountService* GetFor(ProfileIOS* profile);

 private:
  // TypedProfileKeyedServiceFactoryIOS:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;
};

}  // namespace brave_account

#endif  // BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_FACTORY_IOS_H_

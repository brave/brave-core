/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "ios/chrome/browser/shared/model/profile/profile_keyed_service_factory_ios.h"

class ProfileIOS;

namespace web {
class BrowserState;
}

namespace brave_account {
class BraveAccountService;

class BraveAccountServiceFactory : public ProfileKeyedServiceFactoryIOS {
 public:
  BraveAccountServiceFactory(const BraveAccountServiceFactory&) = delete;
  BraveAccountServiceFactory& operator=(const BraveAccountServiceFactory&) =
      delete;

  static BraveAccountServiceFactory* GetInstance();
  static BraveAccountService* GetForProfile(ProfileIOS* profile);

 private:
  friend base::NoDestructor<BraveAccountServiceFactory>;

  BraveAccountServiceFactory();
  ~BraveAccountServiceFactory() override;

  // BrowserStateKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* state) const override;
};
}  // namespace brave_account

#endif  // BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_FACTORY_H_

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_FACTORY_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_FACTORY_BASE_H_

#include "base/check.h"
#include "base/no_destructor.h"

namespace brave_account {
class BraveAccountService;

template <typename BraveAccountServiceFactory,
          typename ProfileKeyedServiceFactory>
class BraveAccountServiceFactoryBase : public ProfileKeyedServiceFactory {
 public:
  BraveAccountServiceFactoryBase(const BraveAccountServiceFactoryBase&) =
      delete;
  BraveAccountServiceFactoryBase& operator=(
      const BraveAccountServiceFactoryBase&) = delete;

  static BraveAccountServiceFactory* GetInstance() {
    static base::NoDestructor<BraveAccountServiceFactory> instance;
    return instance.get();
  }

 protected:
  BraveAccountServiceFactoryBase()
      : ProfileKeyedServiceFactory("BraveAccountService") {}

  ~BraveAccountServiceFactoryBase() override = default;

  static BraveAccountService* GetFor(void* context) {
    CHECK(context);
    return static_cast<BraveAccountService*>(
        GetInstance()->GetServiceForContext(context, true));
  }

 private:
  friend base::NoDestructor<BraveAccountServiceFactory>;
};
}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_FACTORY_BASE_H_

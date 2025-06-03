/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/brave_account/brave_account_service_factory_base.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace content {
class BrowserContext;
}

namespace brave_account {
class BraveAccountService;

class BraveAccountServiceFactory
    : public BraveAccountServiceFactoryBase<BraveAccountServiceFactory,
                                            ProfileKeyedServiceFactory> {
 public:
  static BraveAccountService* GetForBrowserContext(
      content::BrowserContext* context);

 private:
  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};
}  // namespace brave_account

#endif  // BRAVE_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_FACTORY_H_

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DEPENDENT_SERVICES_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DEPENDENT_SERVICES_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class KeyedService;

namespace content {
class BrowserContext;
}

namespace brave_account {

class BraveAccountDependentServices;

class BraveAccountDependentServicesFactory : public ProfileKeyedServiceFactory {
 public:
  BraveAccountDependentServicesFactory(
      const BraveAccountDependentServicesFactory&) = delete;
  BraveAccountDependentServicesFactory& operator=(
      const BraveAccountDependentServicesFactory&) = delete;

  static BraveAccountDependentServicesFactory* GetInstance();
  static BraveAccountDependentServices* GetFor(
      content::BrowserContext* context);

 private:
  friend base::NoDestructor<BraveAccountDependentServicesFactory>;

  BraveAccountDependentServicesFactory();
  ~BraveAccountDependentServicesFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace brave_account

#endif  // BRAVE_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DEPENDENT_SERVICES_FACTORY_H_

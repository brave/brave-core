/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_account/brave_account_dependent_services_factory.h"

#include "base/check.h"
#include "brave/components/brave_account/brave_account_dependent_services.h"
#include "brave/components/brave_account/features.h"
#include "content/public/browser/browser_context.h"

namespace brave_account {

// static
BraveAccountDependentServicesFactory*
BraveAccountDependentServicesFactory::GetInstance() {
  static base::NoDestructor<BraveAccountDependentServicesFactory> instance;
  return instance.get();
}

// static
BraveAccountDependentServices* BraveAccountDependentServicesFactory::GetFor(
    content::BrowserContext* context) {
  CHECK(context);
  return static_cast<BraveAccountDependentServices*>(
      GetInstance()->GetServiceForContext(context, true));
}

BraveAccountDependentServicesFactory::BraveAccountDependentServicesFactory()
    : ProfileKeyedServiceFactory("BraveAccountDependentServices") {
  CHECK(features::IsBraveAccountEnabled());
}

BraveAccountDependentServicesFactory::~BraveAccountDependentServicesFactory() =
    default;

std::unique_ptr<KeyedService>
BraveAccountDependentServicesFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<BraveAccountDependentServices>();
}

}  // namespace brave_account

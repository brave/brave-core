/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_account/brave_account_dependent_services_factory_ios.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_account/brave_account_dependent_services.h"
#include "brave/components/brave_account/features.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace brave_account {

// static
BraveAccountDependentServices* BraveAccountDependentServicesFactoryIOS::GetFor(
    ProfileIOS* profile) {
  CHECK(profile);
  return GetForProfile(profile);
}

BraveAccountDependentServicesFactoryIOS::
    BraveAccountDependentServicesFactoryIOS(PassKey key)
    : TypedProfileKeyedServiceFactoryIOS(std::move(key),
                                         "BraveAccountDependentServices") {
  CHECK(features::IsBraveAccountEnabled());
}

std::unique_ptr<KeyedService>
BraveAccountDependentServicesFactoryIOS::BuildServiceInstanceFor(
    ProfileIOS& profile) const {
  return std::make_unique<BraveAccountDependentServices>();
}

}  // namespace brave_account

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/misc_metrics/profile_misc_metrics_service_factory.h"

#include <memory>

#include "brave/ios/browser/misc_metrics/profile_misc_metrics_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace misc_metrics {

ProfileMiscMetricsServiceFactory::ProfileMiscMetricsServiceFactory(PassKey key)
    : TypedProfileKeyedServiceFactoryIOS(
          std::move(key),
          "ProfileMiscMetricsService",
          ProfileSelection::kNoInstanceInIncognito,
          ServiceCreation::kCreateLazily,
          TestingCreation::kCreateService) {}

std::unique_ptr<KeyedService>
ProfileMiscMetricsServiceFactory::BuildServiceInstanceFor(
    ProfileIOS* profile) const {
  return std::make_unique<ProfileMiscMetricsService>(profile);
}

}  // namespace misc_metrics

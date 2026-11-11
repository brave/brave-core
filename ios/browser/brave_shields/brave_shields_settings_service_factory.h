// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_SETTINGS_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_SETTINGS_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "ios/chrome/browser/shared/model/profile/typed_profile_keyed_service_factory_ios.h"

class ProfileIOS;

namespace brave_shields {

class BraveShieldsSettingsServiceFactory
    : public TypedProfileKeyedServiceFactoryIOS<
          BraveShieldsSettingsServiceFactory,
          BraveShieldsSettingsService> {
 public:
  explicit BraveShieldsSettingsServiceFactory(PassKey key);

  BraveShieldsSettingsServiceFactory(
      const BraveShieldsSettingsServiceFactory&) = delete;
  BraveShieldsSettingsServiceFactory& operator=(
      const BraveShieldsSettingsServiceFactory&) = delete;

 private:
  // TypedProfileKeyedServiceFactoryIOS:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* context) const override;
};
}  // namespace brave_shields

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_SETTINGS_SERVICE_FACTORY_H_

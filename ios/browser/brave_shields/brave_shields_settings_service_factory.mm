// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/brave_shields_settings_service_factory.h"

#include "base/no_destructor.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/content_settings/model/host_content_settings_map_factory.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace brave_shields {

// static
BraveShieldsSettingsService* BraveShieldsSettingsServiceFactory::GetForProfile(
    ProfileIOS* profile) {
  return GetInstance()->GetServiceForProfileAs<BraveShieldsSettingsService>(
      profile, true);
}

// static
BraveShieldsSettingsServiceFactory*
BraveShieldsSettingsServiceFactory::GetInstance() {
  static base::NoDestructor<BraveShieldsSettingsServiceFactory> instance;
  return instance.get();
}

BraveShieldsSettingsServiceFactory::BraveShieldsSettingsServiceFactory()
    : ProfileKeyedServiceFactoryIOS("BraveShieldsSettingsService",
                                    ProfileSelection::kOwnInstanceInIncognito,
                                    ServiceCreation::kCreateLazily,
                                    TestingCreation::kCreateService) {
  DependsOn(ios::HostContentSettingsMapFactory::GetInstance());
}

BraveShieldsSettingsServiceFactory::~BraveShieldsSettingsServiceFactory() {}

std::unique_ptr<KeyedService>
BraveShieldsSettingsServiceFactory::BuildServiceInstanceFor(
    ProfileIOS* profile) const {
  auto* map = ios::HostContentSettingsMapFactory::GetForProfile(profile);
  CHECK(map);
  auto* local_state = GetApplicationContext()->GetLocalState();

  return std::make_unique<BraveShieldsSettingsService>(*map, local_state,
                                                       profile->GetPrefs());
}

}  // namespace brave_shields

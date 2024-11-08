/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_ads/ads_service_factory_ios.h"

#include "base/no_destructor.h"
#include "brave/ios/browser/brave_ads/ads_service_impl_ios.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"

namespace brave_ads {

// static
AdsServiceImplIOS* AdsServiceFactoryIOS::GetForBrowserState(
    ProfileIOS* profile) {
  return static_cast<AdsServiceImplIOS*>(
      GetInstance()->GetServiceForBrowserState(profile, true));
}

// static
AdsServiceFactoryIOS* AdsServiceFactoryIOS::GetInstance() {
  static base::NoDestructor<AdsServiceFactoryIOS> instance;
  return instance.get();
}

AdsServiceFactoryIOS::AdsServiceFactoryIOS()
    : BrowserStateKeyedServiceFactory(
          "AdsService",
          BrowserStateDependencyManager::GetInstance()) {}

AdsServiceFactoryIOS::~AdsServiceFactoryIOS() = default;

std::unique_ptr<KeyedService> AdsServiceFactoryIOS::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ProfileIOS* profile = ProfileIOS::FromBrowserState(context);
  CHECK(profile);
  if (profile->IsOffTheRecord()) {
    return nullptr;
  }

  return std::make_unique<AdsServiceImplIOS>(profile->GetPrefs());
}

}  // namespace brave_ads

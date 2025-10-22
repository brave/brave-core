/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_account/brave_account_service_factory_ios.h"

#include "base/functional/bind.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/features.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

namespace {

std::unique_ptr<KeyedService> BuildBraveAccountService(ProfileIOS* profile) {
  CHECK(profile);
  return std::make_unique<BraveAccountService>(
      profile->GetPrefs(), profile->GetSharedURLLoaderFactory());
}

}  // namespace

// static
BraveAccountServiceFactoryIOS* BraveAccountServiceFactoryIOS::GetInstance() {
  static base::NoDestructor<BraveAccountServiceFactoryIOS> instance;
  return instance.get();
}

// static
BraveAccountService* BraveAccountServiceFactoryIOS::GetFor(
    web::BrowserState* state) {
  CHECK(state);
  return static_cast<BraveAccountService*>(
      GetInstance()->GetServiceForContext(state, true));
}

// static
ProfileKeyedServiceFactoryIOS::TestingFactory
BraveAccountServiceFactoryIOS::GetDefaultFactory() {
  return base::BindOnce(&BuildBraveAccountService);
}

BraveAccountServiceFactoryIOS::BraveAccountServiceFactoryIOS()
    : ProfileKeyedServiceFactoryIOS("BraveAccountService",
                                    ServiceCreation::kCreateWithProfile,
                                    TestingCreation::kNoServiceForTests) {
  CHECK(features::IsBraveAccountEnabled());
}

BraveAccountServiceFactoryIOS::~BraveAccountServiceFactoryIOS() = default;

std::unique_ptr<KeyedService>
BraveAccountServiceFactoryIOS::BuildServiceInstanceFor(
    ProfileIOS* profile) const {
  return BuildBraveAccountService(profile);
}

}  // namespace brave_account

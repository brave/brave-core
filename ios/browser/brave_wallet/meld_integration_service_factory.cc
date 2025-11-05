/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/meld_integration_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/components/brave_wallet/browser/meld_integration_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_manager_ios.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
mojo::PendingRemote<mojom::MeldIntegrationService>
MeldIntegrationServiceFactory::GetForProfile(ProfileIOS* profile) {
  return GetInstance()
      ->GetServiceForProfileAs<MeldIntegrationService>(profile, true)
      ->MakeRemote();
}

// static
MeldIntegrationService* MeldIntegrationServiceFactory::GetServiceForState(
    ProfileIOS* profile) {
  return GetInstance()->GetServiceForProfileAs<MeldIntegrationService>(profile,
                                                                       true);
}

// static
MeldIntegrationServiceFactory* MeldIntegrationServiceFactory::GetInstance() {
  static base::NoDestructor<MeldIntegrationServiceFactory> instance;
  return instance.get();
}

MeldIntegrationServiceFactory::MeldIntegrationServiceFactory()
    : ProfileKeyedServiceFactoryIOS("MeldIntegrationService",
                                    ProfileSelection::kRedirectedInIncognito,
                                    ServiceCreation::kCreateLazily,
                                    TestingCreation::kNoServiceForTests) {}

MeldIntegrationServiceFactory::~MeldIntegrationServiceFactory() = default;

std::unique_ptr<KeyedService>
MeldIntegrationServiceFactory::BuildServiceInstanceFor(
    ProfileIOS* profile) const {
  return std::make_unique<MeldIntegrationService>(
      profile->GetSharedURLLoaderFactory());
}

}  // namespace brave_wallet

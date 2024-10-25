/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/meld_integration_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/components/brave_wallet/browser/meld_integration_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/shared/model/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_manager_ios.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
mojo::PendingRemote<mojom::MeldIntegrationService>
MeldIntegrationServiceFactory::GetForBrowserState(ProfileIOS* profile) {
  return static_cast<MeldIntegrationService*>(
             GetInstance()->GetServiceForBrowserState(profile, true))
      ->MakeRemote();
}

// static
MeldIntegrationService* MeldIntegrationServiceFactory::GetServiceForState(
    ProfileIOS* profile) {
  return static_cast<MeldIntegrationService*>(
      GetInstance()->GetServiceForBrowserState(profile, true));
}

// static
MeldIntegrationServiceFactory* MeldIntegrationServiceFactory::GetInstance() {
  static base::NoDestructor<MeldIntegrationServiceFactory> instance;
  return instance.get();
}

MeldIntegrationServiceFactory::MeldIntegrationServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "MeldIntegrationService",
          BrowserStateDependencyManager::GetInstance()) {}

MeldIntegrationServiceFactory::~MeldIntegrationServiceFactory() = default;

std::unique_ptr<KeyedService>
MeldIntegrationServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* profile = ProfileIOS::FromBrowserState(context);
  return std::make_unique<MeldIntegrationService>(
      profile->GetSharedURLLoaderFactory());
}

bool MeldIntegrationServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* MeldIntegrationServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave_wallet

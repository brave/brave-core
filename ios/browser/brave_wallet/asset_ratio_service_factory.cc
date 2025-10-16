/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/asset_ratio_service_factory.h"

#include "base/no_destructor.h"
#include "brave/components/brave_wallet/browser/asset_ratio_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_manager_ios.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

// static
mojo::PendingRemote<mojom::AssetRatioService>
AssetRatioServiceFactory::GetForProfile(ProfileIOS* profile) {
  return GetInstance()
      ->GetServiceForProfileAs<AssetRatioService>(profile, true)
      ->MakeRemote();
}

// static
AssetRatioService* AssetRatioServiceFactory::GetServiceForState(
    ProfileIOS* profile) {
  return GetInstance()->GetServiceForProfileAs<AssetRatioService>(profile,
                                                                  true);
}

// static
AssetRatioServiceFactory* AssetRatioServiceFactory::GetInstance() {
  static base::NoDestructor<AssetRatioServiceFactory> instance;
  return instance.get();
}

AssetRatioServiceFactory::AssetRatioServiceFactory()
    : ProfileKeyedServiceFactoryIOS("AssetRatioService",
                                    ProfileSelection::kRedirectedInIncognito,
                                    ServiceCreation::kCreateLazily,
                                    TestingCreation::kCreateService) {}

AssetRatioServiceFactory::~AssetRatioServiceFactory() = default;

std::unique_ptr<KeyedService> AssetRatioServiceFactory::BuildServiceInstanceFor(
    ProfileIOS* profile) const {
  std::unique_ptr<AssetRatioService> asset_ratio_service(
      new AssetRatioService(profile->GetSharedURLLoaderFactory()));
  return asset_ratio_service;
}

}  // namespace brave_wallet

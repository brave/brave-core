/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"

#include <memory>

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "components/keyed_service/core/keyed_service.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

class BraveWalletServiceDelegateIos : public BraveWalletServiceDelegate {
 public:
  explicit BraveWalletServiceDelegateIos(ProfileIOS* profile) {
    wallet_base_directory_ = profile->GetStatePath();
    is_private_window_ = profile->IsOffTheRecord();
  }

  base::FilePath GetWalletBaseDirectory() override {
    return wallet_base_directory_;
  }
  bool IsPrivateWindow() override { return is_private_window_; }

 protected:
  base::FilePath wallet_base_directory_;
  bool is_private_window_ = false;
};

// static
BraveWalletService* BraveWalletServiceFactory::GetServiceForState(
    ProfileIOS* profile) {
  return GetInstance()->GetServiceForProfileAs<BraveWalletService>(profile,
                                                                   true);
}

// static
BraveWalletServiceFactory* BraveWalletServiceFactory::GetInstance() {
  static base::NoDestructor<BraveWalletServiceFactory> instance;
  return instance.get();
}

BraveWalletServiceFactory::BraveWalletServiceFactory()
    : ProfileKeyedServiceFactoryIOS("BraveWalletService",
                                    ProfileSelection::kNoInstanceInIncognito,
                                    ServiceCreation::kCreateLazily,
                                    TestingCreation::kNoServiceForTests) {}

BraveWalletServiceFactory::~BraveWalletServiceFactory() = default;

std::unique_ptr<KeyedService>
BraveWalletServiceFactory::BuildServiceInstanceFor(ProfileIOS* profile) const {
  std::unique_ptr<BraveWalletService> service(new BraveWalletService(
      profile->GetSharedURLLoaderFactory(),
      std::make_unique<BraveWalletServiceDelegateIos>(profile),
      profile->GetPrefs(), GetApplicationContext()->GetLocalState()));
  return service;
}

}  // namespace brave_wallet

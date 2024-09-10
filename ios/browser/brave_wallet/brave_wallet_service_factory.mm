/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"

#include <memory>

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

class BraveWalletServiceDelegateIos : public BraveWalletServiceDelegate {
 public:
  explicit BraveWalletServiceDelegateIos(ChromeBrowserState* browser_state) {
    wallet_base_directory_ = browser_state->GetStatePath();
    is_private_window_ = browser_state->IsOffTheRecord();
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
    ChromeBrowserState* browser_state) {
  return static_cast<BraveWalletService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
BraveWalletServiceFactory* BraveWalletServiceFactory::GetInstance() {
  static base::NoDestructor<BraveWalletServiceFactory> instance;
  return instance.get();
}

BraveWalletServiceFactory::BraveWalletServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "BraveWalletService",
          BrowserStateDependencyManager::GetInstance()) {}

BraveWalletServiceFactory::~BraveWalletServiceFactory() = default;

std::unique_ptr<KeyedService>
BraveWalletServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* browser_state = ChromeBrowserState::FromBrowserState(context);
  std::unique_ptr<BraveWalletService> service(new BraveWalletService(
      browser_state->GetSharedURLLoaderFactory(),
      std::make_unique<BraveWalletServiceDelegateIos>(browser_state),
      browser_state->GetPrefs(), GetApplicationContext()->GetLocalState()));
  return service;
}

bool BraveWalletServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* BraveWalletServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave_wallet

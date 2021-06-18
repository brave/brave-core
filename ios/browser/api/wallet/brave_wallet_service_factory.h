/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_WALLET_BRAVE_WALLET_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_API_WALLET_BRAVE_WALLET_SERVICE_FACTORY_H_

#include <memory>

#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

class ChromeBrowserState;

namespace brave_wallet {
class BraveWalletService;
}

class BraveWalletServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  // Creates the service if it doesn't exist already for |browser_state|.
  static brave_wallet::BraveWalletService* GetForBrowserState(
      ChromeBrowserState* browser_state);

  static BraveWalletServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BraveWalletServiceFactory>;

  BraveWalletServiceFactory();
  ~BraveWalletServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(BraveWalletServiceFactory);
};

#endif  // BRAVE_IOS_BROWSER_API_WALLET_BRAVE_WALLET_SERVICE_FACTORY_H_

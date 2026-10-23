/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "ios/chrome/browser/shared/model/profile/typed_profile_keyed_service_factory_ios.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

class ProfileIOS;

namespace brave_wallet {

class BraveWalletServiceFactory
    : public TypedProfileKeyedServiceFactoryIOS<BraveWalletServiceFactory,
                                                BraveWalletService> {
 public:
  static BraveWalletService* GetServiceForState(ProfileIOS* profile);

  explicit BraveWalletServiceFactory(PassKey key);

 private:
  // TypedProfileKeyedServiceFactoryIOS implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;

  BraveWalletServiceFactory(const BraveWalletServiceFactory&) = delete;
  BraveWalletServiceFactory& operator=(const BraveWalletServiceFactory&) =
      delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_FACTORY_H_

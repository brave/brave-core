/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_WALLET_SWAP_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_BRAVE_WALLET_SWAP_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "ios/chrome/browser/shared/model/profile/profile_keyed_service_factory_ios.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

class ProfileIOS;
class KeyedService;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave_wallet {

class SwapService;

class SwapServiceFactory : public ProfileKeyedServiceFactoryIOS {
 public:
  // Creates the service if it doesn't exist already for |profile|.
  static mojo::PendingRemote<mojom::SwapService> GetForProfile(
      ProfileIOS* profile);

  static SwapService* GetServiceForProfile(ProfileIOS* profile);

  static SwapServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<SwapServiceFactory>;

  SwapServiceFactory();
  ~SwapServiceFactory() override;

  // ProfileKeyedServiceFactoryIOS implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;

  SwapServiceFactory(const SwapServiceFactory&) = delete;
  SwapServiceFactory& operator=(const SwapServiceFactory&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_BRAVE_WALLET_SWAP_SERVICE_FACTORY_H_

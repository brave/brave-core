/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_IPFS_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_IPFS_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

class ProfileIOS;
class KeyedService;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace web {
class BrowserState;
}  // namespace web

namespace brave_wallet {

class BraveWalletIpfsService;

class BraveWalletIpfsServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  BraveWalletIpfsServiceFactory(const BraveWalletIpfsServiceFactory&) = delete;
  BraveWalletIpfsServiceFactory& operator=(
      const BraveWalletIpfsServiceFactory&) = delete;

  // Creates the service if it doesn't exist already for |profile|.
  static mojo::PendingRemote<mojom::IpfsService> GetForBrowserState(
      ProfileIOS* profile);
  static BraveWalletIpfsService* GetServiceForState(ProfileIOS* profile);

  static BraveWalletIpfsServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<BraveWalletIpfsServiceFactory>;

  BraveWalletIpfsServiceFactory();
  ~BraveWalletIpfsServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_IPFS_SERVICE_FACTORY_H_

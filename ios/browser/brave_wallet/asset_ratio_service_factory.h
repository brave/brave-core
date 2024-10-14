/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_WALLET_ASSET_RATIO_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_BRAVE_WALLET_ASSET_RATIO_SERVICE_FACTORY_H_

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

class AssetRatioService;

class AssetRatioServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  // Creates the service if it doesn't exist already for |profile|.
  static mojo::PendingRemote<mojom::AssetRatioService> GetForBrowserState(
      ProfileIOS* profile);
  static AssetRatioService* GetServiceForState(ProfileIOS* profile);

  static AssetRatioServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<AssetRatioServiceFactory>;

  AssetRatioServiceFactory();
  ~AssetRatioServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  AssetRatioServiceFactory(const AssetRatioServiceFactory&) = delete;
  AssetRatioServiceFactory& operator=(const AssetRatioServiceFactory&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_BRAVE_WALLET_ASSET_RATIO_SERVICE_FACTORY_H_

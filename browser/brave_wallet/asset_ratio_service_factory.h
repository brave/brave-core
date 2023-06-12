/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_ASSET_RATIO_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_WALLET_ASSET_RATIO_SERVICE_FACTORY_H_

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave_wallet {

class AssetRatioService;

class AssetRatioServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static mojo::PendingRemote<mojom::AssetRatioService> GetForContext(
      content::BrowserContext* context);
  static AssetRatioService* GetServiceForContext(
      content::BrowserContext* context);
  static AssetRatioServiceFactory* GetInstance();
  static void BindForContext(
      content::BrowserContext* context,
      mojo::PendingReceiver<mojom::AssetRatioService> receiver);

 private:
  friend base::NoDestructor<AssetRatioServiceFactory>;

  AssetRatioServiceFactory();
  AssetRatioServiceFactory(const AssetRatioServiceFactory&) = delete;
  AssetRatioServiceFactory& operator=(const AssetRatioServiceFactory&) = delete;

  ~AssetRatioServiceFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_ASSET_RATIO_SERVICE_FACTORY_H_

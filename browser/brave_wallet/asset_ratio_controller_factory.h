/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_ASSET_RATIO_CONTROLLER_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_WALLET_ASSET_RATIO_CONTROLLER_FACTORY_H_

#include "base/memory/singleton.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace brave_wallet {

class AssetRatioController;

class AssetRatioControllerFactory : public BrowserContextKeyedServiceFactory {
 public:
  static mojo::PendingRemote<mojom::AssetRatioController> GetForContext(
      content::BrowserContext* context);
  static AssetRatioController* GetControllerForContext(
      content::BrowserContext* context);
  static AssetRatioControllerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<AssetRatioControllerFactory>;

  AssetRatioControllerFactory();
  AssetRatioControllerFactory(const AssetRatioControllerFactory&) = delete;
  AssetRatioControllerFactory& operator=(const AssetRatioControllerFactory&) =
      delete;

  ~AssetRatioControllerFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_ASSET_RATIO_CONTROLLER_FACTORY_H_

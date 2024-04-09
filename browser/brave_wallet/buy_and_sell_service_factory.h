/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BUY_AND_SELL_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BUY_AND_SELL_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace brave_wallet {

class BuyAndSellService;

class BuyAndSellServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static mojo::PendingRemote<mojom::BuyAndSellService> GetForContext(
      content::BrowserContext* context);
  static BuyAndSellService* GetServiceForContext(
      content::BrowserContext* context);
  static BuyAndSellServiceFactory* GetInstance();
  static void BindForContext(
      content::BrowserContext* context,
      mojo::PendingReceiver<mojom::BuyAndSellService> receiver);

 private:
  friend base::NoDestructor<BuyAndSellServiceFactory>;

  BuyAndSellServiceFactory();
  BuyAndSellServiceFactory(const BuyAndSellServiceFactory&) = delete;
  BuyAndSellServiceFactory& operator=(const BuyAndSellServiceFactory&) = delete;

  ~BuyAndSellServiceFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BUY_AND_SELL_SERVICE_FACTORY_H_

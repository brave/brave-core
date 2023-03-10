/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BITCOIN_RPC_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BITCOIN_RPC_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace brave_wallet {

class BitcoinRpcService;

class BitcoinRpcServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  BitcoinRpcServiceFactory(const BitcoinRpcServiceFactory&) = delete;
  BitcoinRpcServiceFactory& operator=(const BitcoinRpcServiceFactory&) = delete;

  static mojo::PendingRemote<mojom::BitcoinRpcService> GetForContext(
      content::BrowserContext* context);
  static BitcoinRpcService* GetServiceForContext(
      content::BrowserContext* context);
  static BitcoinRpcServiceFactory* GetInstance();
  static void BindForContext(
      content::BrowserContext* context,
      mojo::PendingReceiver<mojom::BitcoinRpcService> receiver);

 private:
  friend struct base::DefaultSingletonTraits<BitcoinRpcServiceFactory>;

  BitcoinRpcServiceFactory();
  ~BitcoinRpcServiceFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BITCOIN_RPC_SERVICE_FACTORY_H_

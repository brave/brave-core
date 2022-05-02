/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_TX_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_WALLET_TX_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace brave_wallet {

class TxService;

class TxServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  TxServiceFactory(const TxServiceFactory&) = delete;
  TxServiceFactory& operator=(const TxServiceFactory&) = delete;

  static mojo::PendingRemote<mojom::TxService> GetForContext(
      content::BrowserContext* context);
  static mojo::PendingRemote<mojom::EthTxManagerProxy>
  GetEthTxManagerProxyForContext(content::BrowserContext* context);
  static mojo::PendingRemote<mojom::SolanaTxManagerProxy>
  GetSolanaTxManagerProxyForContext(content::BrowserContext* context);
  mojo::PendingRemote<mojom::FilTxManagerProxy> GetFilTxManagerProxyForContext(
      content::BrowserContext* context);
  static TxService* GetServiceForContext(content::BrowserContext* context);
  static TxServiceFactory* GetInstance();
  static void BindForContext(content::BrowserContext* context,
                             mojo::PendingReceiver<mojom::TxService> receiver);
  static void BindEthTxManagerProxyForContext(
      content::BrowserContext* context,
      mojo::PendingReceiver<mojom::EthTxManagerProxy> receiver);
  static void BindSolanaTxManagerProxyForContext(
      content::BrowserContext* context,
      mojo::PendingReceiver<mojom::SolanaTxManagerProxy> receiver);
  static void BindFilTxManagerProxyForContext(
      content::BrowserContext* context,
      mojo::PendingReceiver<mojom::FilTxManagerProxy> receiver);

 private:
  friend struct base::DefaultSingletonTraits<TxServiceFactory>;

  TxServiceFactory();
  ~TxServiceFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_TX_SERVICE_FACTORY_H_

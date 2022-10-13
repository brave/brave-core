/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WALLET_CONNECT_WALLET_CONNECT_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_WALLET_CONNECT_WALLET_CONNECT_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "brave/components/wallet_connect/wallet_connect.mojom.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace wallet_connect {

class WalletConnectService;

class WalletConnectServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  WalletConnectServiceFactory(const WalletConnectServiceFactory&) = delete;
  WalletConnectServiceFactory& operator=(const WalletConnectServiceFactory&) =
      delete;

  static mojo::PendingRemote<mojom::WalletConnectService> GetForContext(
      content::BrowserContext* context);
  static WalletConnectService* GetServiceForContext(
      content::BrowserContext* context);
  static WalletConnectServiceFactory* GetInstance();
  static void BindForContext(
      content::BrowserContext* context,
      mojo::PendingReceiver<mojom::WalletConnectService> receiver);

 private:
  friend struct base::DefaultSingletonTraits<WalletConnectServiceFactory>;

  WalletConnectServiceFactory();
  ~WalletConnectServiceFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace wallet_connect

#endif  // BRAVE_BROWSER_WALLET_CONNECT_WALLET_CONNECT_SERVICE_FACTORY_H_

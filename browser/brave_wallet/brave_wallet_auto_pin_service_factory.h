// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_AUTO_PIN_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_AUTO_PIN_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"

#include "brave/components/brave_wallet/browser/brave_wallet_auto_pin_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace brave_wallet {

class BraveWalletAutoPinService;

class BraveWalletAutoPinServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static mojo::PendingRemote<mojom::WalletAutoPinService> GetForContext(
      content::BrowserContext* context);
  static BraveWalletAutoPinService* GetServiceForContext(
      content::BrowserContext* context);
  static BraveWalletAutoPinServiceFactory* GetInstance();
  static void BindForContext(
      content::BrowserContext* context,
      mojo::PendingReceiver<mojom::WalletAutoPinService> receiver);

 private:
  friend struct base::DefaultSingletonTraits<BraveWalletAutoPinServiceFactory>;

  BraveWalletAutoPinServiceFactory();
  ~BraveWalletAutoPinServiceFactory() override;

  BraveWalletAutoPinServiceFactory(const BraveWalletAutoPinServiceFactory&) =
      delete;
  BraveWalletAutoPinServiceFactory& operator=(
      const BraveWalletAutoPinServiceFactory&) = delete;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_AUTO_PIN_SERVICE_FACTORY_H_

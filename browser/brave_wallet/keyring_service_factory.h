/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_KEYRING_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_WALLET_KEYRING_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace brave_wallet {

class KeyringService;

class KeyringServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  KeyringServiceFactory(const KeyringServiceFactory&) = delete;
  KeyringServiceFactory& operator=(const KeyringServiceFactory&) = delete;

  static mojo::PendingRemote<mojom::KeyringService> GetForContext(
      content::BrowserContext* context);
  static KeyringService* GetServiceForContext(content::BrowserContext* context);
  static KeyringServiceFactory* GetInstance();
  static void BindForContext(
      content::BrowserContext* context,
      mojo::PendingReceiver<mojom::KeyringService> receiver);

 private:
  friend struct base::DefaultSingletonTraits<KeyringServiceFactory>;

  KeyringServiceFactory();
  ~KeyringServiceFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_KEYRING_SERVICE_FACTORY_H_

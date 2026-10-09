/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PERMISSIONS_BRAVE_WALLET_ACCOUNT_CHOOSER_CONTEXTS_FACTORY_H_
#define BRAVE_BROWSER_PERMISSIONS_BRAVE_WALLET_ACCOUNT_CHOOSER_CONTEXTS_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace brave_wallet {

class BraveWalletAccountChooserContexts;

class BraveWalletAccountChooserContextsFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static BraveWalletAccountChooserContexts* GetForContext(
      content::BrowserContext* context);
  static BraveWalletAccountChooserContextsFactory* GetInstance();

  BraveWalletAccountChooserContextsFactory(
      const BraveWalletAccountChooserContextsFactory&) = delete;
  BraveWalletAccountChooserContextsFactory& operator=(
      const BraveWalletAccountChooserContextsFactory&) = delete;

 private:
  friend base::NoDestructor<BraveWalletAccountChooserContextsFactory>;

  BraveWalletAccountChooserContextsFactory();
  ~BraveWalletAccountChooserContextsFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_PERMISSIONS_BRAVE_WALLET_ACCOUNT_CHOOSER_CONTEXTS_FACTORY_H_

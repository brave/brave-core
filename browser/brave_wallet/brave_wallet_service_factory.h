/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/brave_wallet/browser/brave_wallet_service_factory_base.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave_wallet {

class BraveWalletService;
class JsonRpcService;

class BraveWalletServiceFactory
    : public BraveWalletServiceFactoryBase<BrowserContextKeyedServiceFactory> {
 public:
  static BraveWalletService* GetServiceForContext(
      content::BrowserContext* context);
  static BraveWalletServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<BraveWalletServiceFactory>;

  BraveWalletServiceFactory();
  ~BraveWalletServiceFactory() override;

  BraveWalletServiceFactory(const BraveWalletServiceFactory&) = delete;
  BraveWalletServiceFactory& operator=(const BraveWalletServiceFactory&) =
      delete;

  scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory(
      content::BrowserContext* context) const override;

  std::unique_ptr<BraveWalletServiceDelegate>
  GetBraveWalletServiceDelegate(content::BrowserContext* context) const override;

  PrefService* GetProfilePrefs(content::BrowserContext* context) const override;

  PrefService* GetLocalState() const override;

  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_FACTORY_H_

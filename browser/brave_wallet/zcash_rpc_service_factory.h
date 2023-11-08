/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_ZCASH_RPC_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_WALLET_ZCASH_RPC_SERVICE_FACTORY_H_

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

class ZCashRpc;

class ZCashRpcServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  ZCashRpcServiceFactory(const ZCashRpcServiceFactory&) = delete;
  ZCashRpcServiceFactory& operator=(const ZCashRpcServiceFactory&) = delete;

  static ZCashRpc* GetServiceForContext(content::BrowserContext* context);
  static ZCashRpcServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<ZCashRpcServiceFactory>;

  ZCashRpcServiceFactory();
  ~ZCashRpcServiceFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_ZCASH_RPC_SERVICE_FACTORY_H_

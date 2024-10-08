/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_IPFS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_IPFS_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/brave_wallet/browser/brave_wallet_ipfs_service.h"
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

class BraveWalletIpfsServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  BraveWalletIpfsServiceFactory(const BraveWalletIpfsServiceFactory&) = delete;
  BraveWalletIpfsServiceFactory& operator=(
      const BraveWalletIpfsServiceFactory&) = delete;

  static mojo::PendingRemote<mojom::IpfsService> GetForContext(
      content::BrowserContext* context);
  static BraveWalletIpfsService* GetServiceForContext(
      content::BrowserContext* context);
  static BraveWalletIpfsServiceFactory* GetInstance();
  static void BindForContext(
      content::BrowserContext* context,
      mojo::PendingReceiver<mojom::IpfsService> receiver);

 private:
  friend base::NoDestructor<BraveWalletIpfsServiceFactory>;

  BraveWalletIpfsServiceFactory();
  ~BraveWalletIpfsServiceFactory() override;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_IPFS_SERVICE_FACTORY_H_

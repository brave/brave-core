/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_DELEGATE_IMPL_ANDROID_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_DELEGATE_IMPL_ANDROID_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"

#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace content {
class BrowserContext;
}

namespace brave_wallet {

class ExternalWalletsImporter;

class BraveWalletServiceDelegateImpl : public BraveWalletServiceDelegate {
 public:
  explicit BraveWalletServiceDelegateImpl(content::BrowserContext* context);
  BraveWalletServiceDelegateImpl(const BraveWalletServiceDelegateImpl&) =
      delete;
  BraveWalletServiceDelegateImpl& operator=(
      const BraveWalletServiceDelegateImpl&) = delete;
  ~BraveWalletServiceDelegateImpl() override;

  void AddPermission(mojom::CoinType coin,
                     const url::Origin& origin,
                     const std::string& account,
                     AddPermissionCallback callback) override;
  void HasPermission(mojom::CoinType coin,
                     const url::Origin& origin,
                     const std::string& account,
                     HasPermissionCallback callback) override;
  void ResetPermission(mojom::CoinType coin,
                       const url::Origin& origin,
                       const std::string& account,
                       ResetPermissionCallback callback) override;
  void IsPermissionDenied(mojom::CoinType coin,
                          const url::Origin& origin,
                          IsPermissionDeniedCallback callback) override;
  void GetWebSitesWithPermission(
      mojom::CoinType coin,
      GetWebSitesWithPermissionCallback callback) override;
  void ResetWebSitePermission(mojom::CoinType coin,
                              const std::string& formed_website,
                              ResetWebSitePermissionCallback callback) override;

 private:
  raw_ptr<content::BrowserContext> context_ = nullptr;
  base::ObserverList<BraveWalletServiceDelegate::Observer> observer_list_;

  base::WeakPtrFactory<BraveWalletServiceDelegateImpl> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_DELEGATE_IMPL_ANDROID_H_

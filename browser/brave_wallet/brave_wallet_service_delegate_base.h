/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_DELEGATE_BASE_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_DELEGATE_BASE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace content {
class BrowserContext;
}

namespace url {
class Origin;
}

namespace brave_wallet {

// Shared BraveWalletServiceDelegate implementation between Desktop and Android.
class BraveWalletServiceDelegateBase : public BraveWalletServiceDelegate {
 public:
  explicit BraveWalletServiceDelegateBase(content::BrowserContext* context);
  BraveWalletServiceDelegateBase(const BraveWalletServiceDelegateBase&) =
      delete;
  BraveWalletServiceDelegateBase& operator=(
      const BraveWalletServiceDelegateBase&) = delete;
  ~BraveWalletServiceDelegateBase() override;

  bool HasPermission(mojom::CoinType coin,
                     const url::Origin& origin,
                     const std::string& account) override;
  bool ResetPermission(mojom::CoinType coin,
                       const url::Origin& origin,
                       const std::string& account) override;
  bool IsPermissionDenied(mojom::CoinType coin,
                          const url::Origin& origin) override;
  void ResetAllPermissions() override;

  base::FilePath GetWalletBaseDirectory() override;
  bool IsPrivateWindow() override;

 protected:
  base::FilePath wallet_base_directory_;
  bool is_private_window_ = false;
  raw_ptr<content::BrowserContext> context_ = nullptr;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_DELEGATE_BASE_H_

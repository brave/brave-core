/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_DELEGATE_H_

#include <vector>

#include "base/functional/callback.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "brave/components/brave_wallet/common/mojom/brave_wallet_common.mojom-forward.h"
#include "url/origin.h"

namespace brave_wallet {

class BraveWalletProviderDelegate {
 public:
  using RequestPermissionsCallback = base::OnceCallback<void(
      mojom::RequestPermissionsError error,
      std::vector<mojom::AccountIdPtr> allowed_accounts)>;

  BraveWalletProviderDelegate() = default;
  BraveWalletProviderDelegate(const BraveWalletProviderDelegate&) = delete;
  BraveWalletProviderDelegate& operator=(const BraveWalletProviderDelegate&) =
      delete;
  virtual ~BraveWalletProviderDelegate();

  virtual bool IsTabVisible() = 0;
  virtual void ShowPanel(const url::Origin& origin) = 0;
  virtual void ShowWalletBackup() = 0;
  virtual void UnlockWallet() = 0;
  virtual void WalletInteractionDetected() = 0;
  virtual void ShowAccountCreation(mojom::CoinType type,
                                   const url::Origin& origin) = 0;
  // All `accounts` must be of the same coin type (inferred from AccountId).
  virtual void RequestPermissions(
      const std::vector<mojom::AccountIdPtr>& accounts,
      const url::Origin& origin,
      RequestPermissionsCallback) = 0;
  virtual bool IsAccountAllowed(const mojom::AccountIdPtr& account) = 0;
  // All `accounts` must be of the same coin type (inferred from AccountId).
  // Returns the subset of `accounts` that are currently allowed.
  virtual std::vector<mojom::AccountIdPtr> GetAllowedAccounts(
      const std::vector<mojom::AccountIdPtr>& accounts) = 0;
  virtual bool IsPermissionDenied(mojom::CoinType type) = 0;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_DELEGATE_H_

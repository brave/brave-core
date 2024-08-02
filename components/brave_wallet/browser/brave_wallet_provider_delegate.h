/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_DELEGATE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "url/origin.h"

namespace brave_wallet {

class BraveWalletProviderDelegate {
 public:
  using RequestPermissionsCallback = base::OnceCallback<void(
      mojom::RequestPermissionsError error,
      const std::optional<std::vector<std::string>>& allowed_accounts)>;

  BraveWalletProviderDelegate() = default;
  BraveWalletProviderDelegate(const BraveWalletProviderDelegate&) = delete;
  BraveWalletProviderDelegate& operator=(const BraveWalletProviderDelegate&) =
      delete;
  virtual ~BraveWalletProviderDelegate() = default;

  virtual bool IsTabVisible() = 0;
  virtual void ShowPanel() = 0;
  virtual void ShowWalletBackup() = 0;
  virtual void UnlockWallet() = 0;
  virtual void WalletInteractionDetected() = 0;
  virtual void ShowWalletOnboarding() = 0;
  virtual void ShowAccountCreation(mojom::CoinType type) = 0;
  virtual url::Origin GetOrigin() const = 0;
  virtual void RequestPermissions(mojom::CoinType type,
                                  const std::vector<std::string>& accounts,
                                  RequestPermissionsCallback) = 0;
  virtual bool IsAccountAllowed(mojom::CoinType type,
                                const std::string& account) = 0;
  virtual std::optional<std::vector<std::string>> GetAllowedAccounts(
      mojom::CoinType type,
      const std::vector<std::string>& accounts) = 0;
  virtual bool IsPermissionDenied(mojom::CoinType type) = 0;
  virtual void AddSolanaConnectedAccount(const std::string& account) {}
  virtual void RemoveSolanaConnectedAccount(const std::string& account) {}
  virtual bool IsSolanaAccountConnected(const std::string& account);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_DELEGATE_H_

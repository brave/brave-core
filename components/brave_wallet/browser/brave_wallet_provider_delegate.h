/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_DELEGATE_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "url/origin.h"

namespace brave_wallet {

class BraveWalletProviderDelegate {
 public:
  using RequestEthereumPermissionsCallback =
      base::OnceCallback<void(const std::vector<std::string>&,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  using GetAllowedAccountsCallback =
      base::OnceCallback<void(const std::vector<std::string>&,
                              mojom::ProviderError error,
                              const std::string& error_message)>;

  BraveWalletProviderDelegate() = default;
  BraveWalletProviderDelegate(const BraveWalletProviderDelegate&) = delete;
  BraveWalletProviderDelegate& operator=(const BraveWalletProviderDelegate&) =
      delete;
  virtual ~BraveWalletProviderDelegate() = default;

  virtual void ShowPanel() = 0;
  virtual url::Origin GetOrigin() const = 0;
  virtual void RequestEthereumPermissions(
      RequestEthereumPermissionsCallback callback) = 0;
  virtual void GetAllowedAccounts(bool include_accounts_when_locked,
                                  GetAllowedAccountsCallback callback) = 0;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_DELEGATE_H_

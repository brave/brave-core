/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_DELEGATE_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "url/gurl.h"

namespace brave_wallet {

class SolanaProviderDelegate {
 public:
  using RequestSolanaPermissionCallback =
      base::OnceCallback<void(const std::string&,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  using IsSelectedAccountAllowedCallback =
      base::OnceCallback<void(const absl::optional<std::string>&,
                              bool allowed)>;

  SolanaProviderDelegate() = default;
  SolanaProviderDelegate(const SolanaProviderDelegate&) = delete;
  SolanaProviderDelegate& operator=(const SolanaProviderDelegate&) = delete;
  virtual ~SolanaProviderDelegate() = default;

  // TODO(darkdh): rename BraveWalletProviderDelegate to
  // EthereumProviderDelegate and make SolanaProviderDelegate &
  // EthereumProviderDelegate subclass BraveWalletProviderDelegate which has
  // ShowPanel and GetOrigin
  virtual void ShowPanel() = 0;
  virtual GURL GetOrigin() const = 0;
  virtual void RequestSolanaPermission(
      RequestSolanaPermissionCallback callback) = 0;
  virtual void IsSelectedAccountAllowed(
      IsSelectedAccountAllowedCallback callback) = 0;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_DELEGATE_H_

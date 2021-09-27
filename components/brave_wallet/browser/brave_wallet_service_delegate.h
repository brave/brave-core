/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_DELEGATE_H_

#include <string>

#include "base/callback.h"

namespace brave_wallet {

class BraveWalletServiceDelegate {
 public:
  using IsCryptoWalletsInstalledCallback = base::OnceCallback<void(bool)>;
  using IsMetaMaskInstalledCallback = base::OnceCallback<void(bool)>;
  using ImportFromCryptoWalletsCallback = base::OnceCallback<void(bool)>;
  using ImportFromMetaMaskCallback = base::OnceCallback<void(bool)>;
  using HasEthereumPermissionCallback = base::OnceCallback<void(bool, bool)>;
  using ResetEthereumPermissionCallback = base::OnceCallback<void(bool)>;

  BraveWalletServiceDelegate() = default;
  BraveWalletServiceDelegate(const BraveWalletServiceDelegate&) = delete;
  BraveWalletServiceDelegate& operator=(const BraveWalletServiceDelegate&) =
      delete;
  virtual ~BraveWalletServiceDelegate() = default;

  virtual void IsCryptoWalletsInstalled(
      IsCryptoWalletsInstalledCallback callback);
  virtual void IsMetaMaskInstalled(IsMetaMaskInstalledCallback callback);
  virtual void ImportFromCryptoWallets(
      const std::string& password,
      const std::string& new_password,
      ImportFromCryptoWalletsCallback callback);
  virtual void ImportFromMetaMask(const std::string& password,
                                  const std::string& new_password,
                                  ImportFromMetaMaskCallback callback);
  virtual void HasEthereumPermission(const std::string& origin,
                                     const std::string& account,
                                     HasEthereumPermissionCallback callback);
  virtual void ResetEthereumPermission(
      const std::string& origin,
      const std::string& account,
      ResetEthereumPermissionCallback callback);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_DELEGATE_H_

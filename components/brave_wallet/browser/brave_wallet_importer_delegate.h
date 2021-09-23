/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_IMPORTER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_IMPORTER_DELEGATE_H_

#include <string>

#include "base/callback.h"

namespace brave_wallet {

class BraveWalletImporterDelegate {
 public:
  using IsCryptoWalletsInstalledCallback = base::OnceCallback<void(bool)>;
  using IsMetaMaskInstalledCallback = base::OnceCallback<void(bool)>;
  using ImportFromCryptoWalletsCallback = base::OnceCallback<void(bool)>;
  using ImportFromMetaMaskCallback = base::OnceCallback<void(bool)>;

  BraveWalletImporterDelegate() = default;
  BraveWalletImporterDelegate(const BraveWalletImporterDelegate&) = delete;
  BraveWalletImporterDelegate& operator=(const BraveWalletImporterDelegate&) =
      delete;
  virtual ~BraveWalletImporterDelegate() = default;

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
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_IMPORTER_DELEGATE_H_

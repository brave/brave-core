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
  using IsBraveCryptoWalletInstalledCallback = base::OnceCallback<void(bool)>;
  using IsMetamaskInstalledCallback = base::OnceCallback<void(bool)>;
  using ImportFromBraveCryptoWalletCallback = base::OnceCallback<void(bool)>;
  using ImportFromMetamaskCallback = base::OnceCallback<void(bool)>;

  BraveWalletImporterDelegate() = default;
  BraveWalletImporterDelegate(const BraveWalletImporterDelegate&) = delete;
  BraveWalletImporterDelegate& operator=(const BraveWalletImporterDelegate&) =
      delete;
  virtual ~BraveWalletImporterDelegate() = default;

  virtual void IsBraveCryptoWalletInstalled(
      IsBraveCryptoWalletInstalledCallback callback);
  virtual void IsMetamaskInstalled(IsMetamaskInstalledCallback callback);
  virtual void ImportFromBraveCryptoWallet(
      const std::string& password,
      const std::string& new_password,
      ImportFromBraveCryptoWalletCallback callback);
  virtual void ImportFromMetamask(const std::string& password,
                                  const std::string& new_password,
                                  ImportFromMetamaskCallback callback);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_IMPORTER_DELEGATE_H_

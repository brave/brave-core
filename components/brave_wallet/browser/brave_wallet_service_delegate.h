/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_DELEGATE_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/observer_list_types.h"

namespace content {
class BrowserContext;
}

namespace brave_wallet {

class BraveWalletServiceDelegate {
 public:
  struct ImportInfo {
    std::string mnemonic;
    bool is_legacy_crypto_wallets;
  };

  using IsCryptoWalletsInstalledCallback = base::OnceCallback<void(bool)>;
  using IsMetaMaskInstalledCallback = base::OnceCallback<void(bool)>;
  using GetImportInfoCallback = base::OnceCallback<void(bool, ImportInfo)>;
  using HasEthereumPermissionCallback = base::OnceCallback<void(bool, bool)>;
  using ResetEthereumPermissionCallback = base::OnceCallback<void(bool)>;
  using GetActiveOriginCallback = base::OnceCallback<void(const std::string&)>;

  BraveWalletServiceDelegate() = default;
  BraveWalletServiceDelegate(const BraveWalletServiceDelegate&) = delete;
  BraveWalletServiceDelegate& operator=(const BraveWalletServiceDelegate&) =
      delete;
  virtual ~BraveWalletServiceDelegate() = default;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnActiveOriginChanged(const std::string& origin) {}
  };
  virtual void AddObserver(Observer* observer) {}
  virtual void RemoveObserver(Observer* observer) {}

  virtual void IsCryptoWalletsInstalled(
      IsCryptoWalletsInstalledCallback callback);
  virtual void IsMetaMaskInstalled(IsMetaMaskInstalledCallback callback);
  virtual void GetImportInfoFromCryptoWallets(const std::string& password,
                                              GetImportInfoCallback callback);
  virtual void GetImportInfoFromMetaMask(const std::string& password,
                                         GetImportInfoCallback callback);
  virtual void HasEthereumPermission(const std::string& origin,
                                     const std::string& account,
                                     HasEthereumPermissionCallback callback);
  virtual void ResetEthereumPermission(
      const std::string& origin,
      const std::string& account,
      ResetEthereumPermissionCallback callback);

  virtual void GetActiveOrigin(GetActiveOriginCallback callback);

  static std::unique_ptr<BraveWalletServiceDelegate> Create(
      content::BrowserContext* browser_context);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_DELEGATE_H_

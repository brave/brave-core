/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_EXTERNAL_WALLETS_IMPORTER_H_
#define BRAVE_BROWSER_BRAVE_WALLET_EXTERNAL_WALLETS_IMPORTER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace content {
class BrowserContext;
}

namespace extensions {
class Extension;
}

namespace brave_wallet {

// This is used to import data from Crypto Wallets and MetaMask, its lifetime
// should be bound to the end of each BraveWalletServiceDelegateImpl function
// calls.
class ExternalWalletsImporter {
 public:
  using InitCallback = base::OnceCallback<void(bool)>;
  using GetImportInfoCallback =
      base::OnceCallback<void(bool, ImportInfo, ImportError)>;

  explicit ExternalWalletsImporter(mojom::ExternalWalletType,
                                   content::BrowserContext*);
  ~ExternalWalletsImporter();
  ExternalWalletsImporter(const ExternalWalletsImporter&) = delete;
  ExternalWalletsImporter& operator=(const ExternalWalletsImporter&) = delete;

  // callback will be call when initialization is finished.
  void Initialize(InitCallback);
  bool IsInitialized() const;

  bool IsExternalWalletInstalled() const;
  bool IsExternalWalletInitialized() const;
  void GetImportInfo(const std::string& password, GetImportInfoCallback) const;

  void SetStorageDataForTesting(std::unique_ptr<base::DictionaryValue>);
  void SetExternalWalletInstalledForTesting(bool installed);

 private:
  const extensions::Extension* GetCryptoWallets() const;
  const extensions::Extension* GetMetaMask() const;
  void OnCryptoWalletsLoaded(InitCallback);
  bool IsCryptoWalletsInstalledInternal() const;

  void GetLocalStorage(const extensions::Extension*, InitCallback);
  void OnGetLocalStorage(InitCallback, std::unique_ptr<base::DictionaryValue>);

  void GetMnemonic(bool is_legacy_crypto_wallets,
                   GetImportInfoCallback callback,
                   const std::string& password) const;

  bool is_external_wallet_installed_for_testing_ = false;
  mojom::ExternalWalletType type_;
  raw_ptr<content::BrowserContext> context_ = nullptr;
  std::unique_ptr<base::DictionaryValue> storage_data_;
  scoped_refptr<extensions::Extension> extension_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<ExternalWalletsImporter> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_EXTERNAL_WALLETS_IMPORTER_H_

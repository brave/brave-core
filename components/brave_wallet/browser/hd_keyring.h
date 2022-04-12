/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEYRING_H_

#include "brave/components/brave_wallet/browser/internal/hd_key.h"

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

class EthTransaction;

class HDKeyring {
 public:
  HDKeyring();
  virtual ~HDKeyring();
  HDKeyring(const HDKeyring&) = delete;
  HDKeyring& operator=(const HDKeyring&) = delete;

  virtual void ConstructRootHDKey(const std::vector<uint8_t>& seed,
                                  const std::string& hd_path);

  virtual void AddAccounts(size_t number);
  // This will return vector of address of all accounts
  std::vector<std::string> GetAccounts() const;
  absl::optional<size_t> GetAccountIndex(const std::string& address) const;
  size_t GetAccountsNumber() const;
  // Only support removing accounts from the back to prevents gaps
  void RemoveAccount();

  // address will be returned
  virtual std::string ImportAccount(const std::vector<uint8_t>& private_key);
  size_t GetImportedAccountsNumber() const;
  bool RemoveImportedAccount(const std::string& address);

  std::string GetAddress(size_t index) const;
  std::string GetDiscoveryAddress(size_t index) const;
  // Find private key by address (it would be hex or base58 depends on
  // underlying hd key
  std::string GetEncodedPrivateKey(const std::string& address);

  std::vector<uint8_t> SignMessage(const std::string& address,
                                   const std::vector<uint8_t>& message);

 protected:
  // Bitcoin keyring can override this for different address calculation
  virtual std::string GetAddressInternal(HDKeyBase* hd_key) const = 0;
  bool AddImportedAddress(const std::string& address,
                          std::unique_ptr<HDKeyBase> hd_key);
  HDKeyBase* GetHDKeyFromAddress(const std::string& address);

  std::unique_ptr<HDKeyBase> root_;
  std::unique_ptr<HDKeyBase> master_key_;
  std::vector<std::unique_ptr<HDKeyBase>> accounts_;
  // (address, key)
  base::flat_map<std::string, std::unique_ptr<HDKeyBase>> imported_accounts_;

 private:
  FRIEND_TEST_ALL_PREFIXES(EthereumKeyringUnitTest, ConstructRootHDKey);
  FRIEND_TEST_ALL_PREFIXES(EthereumKeyringUnitTest, SignMessage);
  FRIEND_TEST_ALL_PREFIXES(SolanaKeyringUnitTest, ConstructRootHDKey);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEYRING_H_

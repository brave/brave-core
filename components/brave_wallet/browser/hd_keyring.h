/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEYRING_H_

#include "brave/components/brave_wallet/browser/internal/hd_key.h"

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"

namespace brave_wallet {

struct AddedAcountInfo {
  uint32_t account_index;
  std::string address;
};

class HDKeyring {
 public:
  HDKeyring();
  virtual ~HDKeyring();
  HDKeyring(const HDKeyring&) = delete;
  HDKeyring& operator=(const HDKeyring&) = delete;

  virtual void ConstructRootHDKey(const std::vector<uint8_t>& seed,
                                  const std::string& hd_path);

  std::vector<AddedAcountInfo> AddAccounts(size_t number);
  // This will return vector of address of all accounts
  std::vector<std::string> GetAccountsForTesting() const;
  // Only support removing accounts from the back to prevents gaps
  void RemoveAccount();

  // address will be returned
  virtual std::string ImportAccount(const std::vector<uint8_t>& private_key);
  size_t GetImportedAccountsNumber() const;
  bool RemoveImportedAccount(const std::string& address);

  std::string GetAddress(size_t index) const;
  std::string GetDiscoveryAddress(size_t index) const;
  // Find private key by address and encode for export (it would be hex or
  // base58 depends on underlying hd key)
  virtual std::string EncodePrivateKeyForExport(const std::string& address);

  bool HasAddress(const std::string& addr);

  bool HasImportedAddress(const std::string& addr);

 protected:
  // Bitcoin keyring can override this for different address calculation
  virtual std::string GetAddressInternal(HDKeyBase* hd_key) const = 0;
  virtual std::unique_ptr<HDKeyBase> DeriveAccount(uint32_t index) const = 0;

  bool AddImportedAddress(const std::string& address,
                          std::unique_ptr<HDKeyBase> hd_key);
  HDKeyBase* GetHDKeyFromAddress(const std::string& address);

  std::unique_ptr<HDKeyBase> root_;
  std::vector<std::unique_ptr<HDKeyBase>> accounts_;
  // TODO(apaymyshev): make separate abstraction for imported keys as they are
  // not HD keys.
  // (address, key)
  base::flat_map<std::string, std::unique_ptr<HDKeyBase>> imported_accounts_;

 private:
  FRIEND_TEST_ALL_PREFIXES(EthereumKeyringUnitTest, ConstructRootHDKey);
  FRIEND_TEST_ALL_PREFIXES(EthereumKeyringUnitTest, SignMessage);
  FRIEND_TEST_ALL_PREFIXES(SolanaKeyringUnitTest, ConstructRootHDKey);
  FRIEND_TEST_ALL_PREFIXES(BitcoinKeyringUnitTest, TestVectors);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEYRING_H_

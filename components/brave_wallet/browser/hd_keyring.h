/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEYRING_H_

#include "brave/components/brave_wallet/browser/hd_key.h"

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"

namespace brave_wallet {

class EthTransaction;

FORWARD_DECLARE_TEST(HDKeyringUnitTest, ConstructRootHDKey);
FORWARD_DECLARE_TEST(HDKeyringUnitTest, SignMessage);
class HDKeyring {
 public:
  enum Type { kDefault = 0, kLedger, kTrezor, kBitcoin };

  HDKeyring();
  virtual ~HDKeyring();

  virtual Type type() const;
  virtual bool empty() const;
  virtual void ClearData();
  virtual void ConstructRootHDKey(const std::vector<uint8_t>& seed,
                                  const std::string& hd_path);

  virtual void AddAccounts(size_t number = 1);
  // This will return vector of address of all accounts
  virtual std::vector<std::string> GetAccounts();
  virtual void RemoveAccount(const std::string& address);

  // Bitcoin keyring can override this for different address calculation
  virtual std::string GetAddress(size_t index);

  // TODO(darkdh): Abstract Transacation class
  // eth_signTransaction
  virtual void SignTransaction(const std::string& address, EthTransaction* tx);
  // eth_sign
  virtual std::vector<uint8_t> SignMessage(const std::string& address,
                                           const std::vector<uint8_t>& message);

 protected:
  HDKey* GetHDKeyFromAddress(const std::string& address);

  std::unique_ptr<HDKey> root_;
  std::unique_ptr<HDKey> master_key_;
  std::vector<std::unique_ptr<HDKey>> accounts_;

 private:
  FRIEND_TEST_ALL_PREFIXES(HDKeyringUnitTest, ConstructRootHDKey);
  FRIEND_TEST_ALL_PREFIXES(HDKeyringUnitTest, SignMessage);

  HDKeyring(const HDKeyring&) = delete;
  HDKeyring& operator=(const HDKeyring&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEYRING_H_

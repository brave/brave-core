/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SECP256K1_HD_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SECP256K1_HD_KEYRING_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/browser/internal/hd_key.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

// Base class for ECDSA over the Secp256k1 types of HD keyrings.
class Secp256k1HDKeyring {
 public:
  Secp256k1HDKeyring();
  virtual ~Secp256k1HDKeyring();

  std::optional<std::string> AddNewHDAccount(uint32_t index);
  bool RemoveHDAccount(uint32_t index);

  std::optional<std::string> ImportAccount(
      base::span<const uint8_t> private_key);
  bool RemoveImportedAccount(const std::string& address);

  std::vector<std::string> GetHDAccountsForTesting() const;
  std::vector<std::string> GetImportedAccountsForTesting() const;

 protected:
  FRIEND_TEST_ALL_PREFIXES(EthereumKeyringUnitTest, SignMessage);

  virtual std::string GetAddressInternal(const HDKey& hd_key) const = 0;
  virtual std::unique_ptr<HDKey> DeriveAccount(uint32_t index) const = 0;

  HDKey* GetHDKeyFromAddress(const std::string& address);

  std::unique_ptr<HDKey> accounts_root_;
  std::vector<std::unique_ptr<HDKey>> accounts_;

  // TODO(apaymyshev): make separate abstraction for imported keys as they are
  // not HD keys.
  // (address, key)
  base::flat_map<std::string, std::unique_ptr<HDKey>> imported_accounts_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SECP256K1_HD_KEYRING_H_

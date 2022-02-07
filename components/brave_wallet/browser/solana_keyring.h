/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_KEYRING_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/hd_keyring.h"

namespace brave_wallet {

class SolanaKeyring : public HDKeyring {
 public:
  SolanaKeyring() = default;
  ~SolanaKeyring() override = default;
  SolanaKeyring(const SolanaKeyring&) = delete;
  SolanaKeyring& operator=(const SolanaKeyring&) = delete;

  Type type() const override;
  void ConstructRootHDKey(const std::vector<uint8_t>& seed,
                          const std::string& hd_path) override;
  void AddAccounts(size_t number = 1) override;

  std::vector<uint8_t> SignMessage(
      const std::string& address,
      const std::vector<uint8_t>& message) override;

  std::string ImportAccount(const std::vector<uint8_t>& private_key) override;

 private:
  std::string GetAddressInternal(HDKeyBase* hd_key) const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_KEYRING_H_

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_KEYRING_H_

#include <memory>
#include <optional>
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

  void ConstructRootHDKey(const std::vector<uint8_t>& seed,
                          const std::string& hd_path) override;

  std::string ImportAccount(const std::vector<uint8_t>& keypair) override;

  std::vector<uint8_t> SignMessage(const std::string& address,
                                   const std::vector<uint8_t>& message);

  static std::optional<std::string> CreateProgramDerivedAddress(
      const std::vector<std::vector<uint8_t>>& seeds,
      const std::string& program_id);

  static std::optional<std::string> FindProgramDerivedAddress(
      const std::vector<std::vector<uint8_t>>& seeds,
      const std::string& program_id,
      uint8_t* bump_seed = nullptr);

  static std::optional<std::string> GetAssociatedTokenAccount(
      const std::string& spl_token_mint_address,
      const std::string& wallet_address);

  static std::optional<std::string> GetAssociatedMetadataAccount(
      const std::string& token_mint_address);

 private:
  std::string GetAddressInternal(HDKeyBase* hd_key) const override;
  std::unique_ptr<HDKeyBase> DeriveAccount(uint32_t index) const override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_KEYRING_H_

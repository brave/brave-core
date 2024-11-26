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

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_ed25519.h"

namespace brave_wallet {

class SolanaKeyring : public HDKeyring {
 public:
  explicit SolanaKeyring(base::span<const uint8_t> seed);
  ~SolanaKeyring() override;
  SolanaKeyring(const SolanaKeyring&) = delete;
  SolanaKeyring& operator=(const SolanaKeyring&) = delete;

  static std::unique_ptr<HDKeyEd25519> ConstructRootHDKey(
      base::span<const uint8_t> seed,
      const std::string& hd_path);

  std::optional<AddedAccountInfo> AddNewHDAccount() override;
  void RemoveLastHDAccount() override;
  std::string ImportAccount(base::span<const uint8_t> payload) override;
  bool RemoveImportedAccount(const std::string& address) override;

  std::string EncodePrivateKeyForExport(const std::string& address) override;

  std::vector<uint8_t> SignMessage(const std::string& address,
                                   base::span<const uint8_t> message);

  static std::optional<std::string> CreateProgramDerivedAddress(
      const std::vector<std::vector<uint8_t>>& seeds,
      const std::string& program_id);

  static std::optional<std::string> FindProgramDerivedAddress(
      const std::vector<std::vector<uint8_t>>& seeds,
      const std::string& program_id,
      uint8_t* bump_seed = nullptr);

  static std::optional<std::string> GetAssociatedTokenAccount(
      const std::string& spl_token_mint_address,
      const std::string& wallet_address,
      mojom::SPLTokenProgram spl_token_program);

  static std::optional<std::string> GetAssociatedMetadataAccount(
      const std::string& token_mint_address);

  std::string GetDiscoveryAddress(size_t index) const override;

  std::vector<std::string> GetHDAccountsForTesting() const override;
  std::vector<std::string> GetImportedAccountsForTesting() const override;

 private:
  FRIEND_TEST_ALL_PREFIXES(SolanaKeyringUnitTest, ConstructRootHDKey);

  std::string GetAddressInternal(const HDKeyEd25519& hd_key) const;
  std::unique_ptr<HDKeyEd25519> DeriveAccount(uint32_t index) const;
  HDKeyEd25519* GetHDKeyFromAddress(const std::string& address);

  std::unique_ptr<HDKeyEd25519> root_;
  std::vector<std::unique_ptr<HDKeyEd25519>> accounts_;

  // TODO(apaymyshev): make separate abstraction for imported keys as they are
  // not HD keys.
  // (address, key)
  base::flat_map<std::string, std::unique_ptr<HDKeyEd25519>> imported_accounts_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_KEYRING_H_

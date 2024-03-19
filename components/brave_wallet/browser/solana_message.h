/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_MESSAGE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_MESSAGE_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/browser/solana_message_address_table_lookup.h"
#include "brave/components/brave_wallet/browser/solana_message_header.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class SolanaMessage {
 public:
  SolanaMessage(
      mojom::SolanaMessageVersion version,
      const std::string& recent_blockhash,
      uint64_t last_valid_block_height,
      const std::string& fee_payer,
      const SolanaMessageHeader& message_header,
      std::vector<SolanaAddress>&& static_account_keys,
      std::vector<SolanaInstruction>&& instructions,
      std::vector<SolanaMessageAddressTableLookup>&& addr_table_lookups);
  SolanaMessage(const SolanaMessage&) = delete;
  SolanaMessage(SolanaMessage&&);
  SolanaMessage& operator=(const SolanaMessage&) = delete;
  SolanaMessage& operator=(SolanaMessage&&);
  ~SolanaMessage();
  bool operator==(const SolanaMessage&) const;

  static std::optional<SolanaMessage> CreateLegacyMessage(
      const std::string& recent_blockhash,
      uint64_t last_valid_block_height,
      const std::string& fee_payer,
      std::vector<SolanaInstruction>&& instructions);

  std::optional<std::vector<uint8_t>> Serialize(
      std::vector<std::string>* signers) const;

  static std::optional<SolanaMessage> Deserialize(
      const std::vector<uint8_t>& bytes);
  static std::optional<std::vector<std::string>>
  GetSignerAccountsFromSerializedMessage(
      const std::vector<uint8_t>& serialized_message);

  void set_recent_blockhash(const std::string& recent_blockhash) {
    recent_blockhash_ = recent_blockhash;
  }
  std::string recent_blockhash() { return recent_blockhash_; }

  void set_last_valid_block_height(uint64_t last_valid_block_height) {
    last_valid_block_height_ = last_valid_block_height;
  }
  uint64_t last_valid_block_height() const { return last_valid_block_height_; }
  std::string fee_payer() const { return fee_payer_; }

  mojom::SolanaTxDataPtr ToSolanaTxData() const;
  base::Value::Dict ToValue() const;

  static std::optional<SolanaMessage> FromValue(const base::Value::Dict& value);
  static std::optional<SolanaMessage> FromDeprecatedLegacyValue(
      const base::Value::Dict& value);

  void SetInstructionsForTesting(
      const std::vector<SolanaInstruction>& instructions) {
    instructions_ = instructions;
  }
  const std::vector<SolanaInstruction>& instructions() const {
    return instructions_;
  }

  mojom::SolanaMessageVersion version() const { return version_; }

  // Returns true if transaction begins with a valid advance nonce instruction.
  // https://docs.rs/solana-sdk/1.18.9/src/solana_sdk/transaction/versioned/mod.rs.html#192
  bool UsesDurableNonce() const;

 private:
  FRIEND_TEST_ALL_PREFIXES(SolanaMessageUnitTest, GetUniqueAccountMetas);

  static void GetUniqueAccountMetas(
      const std::string& fee_payer,
      const std::vector<SolanaInstruction>& instructions,
      std::vector<SolanaAccountMeta>* unique_account_metas);

  mojom::SolanaMessageVersion version_;
  std::string recent_blockhash_;
  uint64_t last_valid_block_height_ = 0;

  // The account responsible for paying the cost of executing a transaction.
  std::string fee_payer_;

  // Describe how many signed accounts, readonly signed accounts, readonly
  // unsigned accounts are in the static_account_keys. Note that it describes
  // static accounts only and does not describe accounts loaded via address
  // table lookups.
  SolanaMessageHeader message_header_;

  // Sorted by signer-writable, signer-readonly, non-signer-writable,
  // non-signer-readonly. If two accounts have same is_signer and is_writable
  // properties, keep them as the insertion order.
  std::vector<SolanaAddress> static_account_keys_;
  std::vector<SolanaInstruction> instructions_;
  std::vector<SolanaMessageAddressTableLookup>
      address_table_lookups_;  // Empty for legacy transactions.
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_MESSAGE_H_

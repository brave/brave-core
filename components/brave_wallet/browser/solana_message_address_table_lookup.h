/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_MESSAGE_ADDRESS_TABLE_LOOKUP_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_MESSAGE_ADDRESS_TABLE_LOOKUP_H_

#include <optional>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/solana_address.h"

namespace brave_wallet {

// An on-chain address lookup table to use for loading more writable and
// readonly accounts in the transaction.
// https://docs.rs/solana-sdk/1.14.12/solana_sdk/message/v0/struct.MessageAddressTableLookup.html
class SolanaMessageAddressTableLookup {
 public:
  SolanaMessageAddressTableLookup(const SolanaAddress& account_key,
                                  const std::vector<uint8_t>& write_indexes,
                                  const std::vector<uint8_t>& read_indexes);

  SolanaMessageAddressTableLookup(const SolanaMessageAddressTableLookup&) =
      delete;
  SolanaMessageAddressTableLookup(SolanaMessageAddressTableLookup&&);
  SolanaMessageAddressTableLookup& operator=(
      const SolanaMessageAddressTableLookup&) = delete;
  SolanaMessageAddressTableLookup& operator=(SolanaMessageAddressTableLookup&&);

  ~SolanaMessageAddressTableLookup();
  bool operator==(const SolanaMessageAddressTableLookup&) const;

  void Serialize(std::vector<uint8_t>* bytes) const;
  static std::optional<SolanaMessageAddressTableLookup> Deserialize(
      const std::vector<uint8_t>& bytes,
      size_t* bytes_index);

  base::Value::Dict ToValue() const;
  static std::optional<SolanaMessageAddressTableLookup> FromValue(
      const base::Value::Dict& value);
  static std::vector<mojom::SolanaMessageAddressTableLookupPtr> ToMojomArray(
      const std::vector<SolanaMessageAddressTableLookup>&
          address_table_lookups);
  static std::optional<std::vector<SolanaMessageAddressTableLookup>>
  FromMojomArray(const std::vector<mojom::SolanaMessageAddressTableLookupPtr>&
                     mojom_lookups);

  const SolanaAddress& account_key() const { return account_key_; }
  const std::vector<uint8_t>& write_indexes() const { return write_indexes_; }
  const std::vector<uint8_t>& read_indexes() const { return read_indexes_; }

 private:
  // Address table lookup account key.
  SolanaAddress account_key_;

  // Indexes to load writable and readable account addresses.
  std::vector<uint8_t> write_indexes_;
  std::vector<uint8_t> read_indexes_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_MESSAGE_ADDRESS_TABLE_LOOKUP_H_

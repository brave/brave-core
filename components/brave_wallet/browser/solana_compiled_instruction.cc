/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_compiled_instruction.h"

#include <optional>

#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/browser/solana_message_address_table_lookup.h"
#include "brave/components/brave_wallet/common/solana_address.h"
#include "brave/components/brave_wallet/common/solana_utils.h"

namespace brave_wallet {

namespace {

std::optional<uint8_t> FindIndexInStaticAccounts(
    const std::vector<SolanaAddress>& keys,
    const std::string& target_key) {
  if (keys.size() > UINT8_MAX) {
    return std::nullopt;
  }
  auto it = base::ranges::find_if(keys, [&](const SolanaAddress& key) {
    return key.ToBase58() == target_key;
  });
  if (it == keys.end()) {
    return std::nullopt;
  }
  return it - keys.begin();
}

std::optional<uint8_t> FindIndexInAddressTableLookups(
    const std::vector<SolanaMessageAddressTableLookup>& addr_table_lookups,
    const SolanaAccountMeta& account,
    uint8_t num_of_static_accounts,
    uint8_t num_of_total_write_indexes) {
  uint8_t index_of_combined_array =
      account.is_writable ? num_of_static_accounts
                          : num_of_static_accounts + num_of_total_write_indexes;

  for (const auto& addr_table_lookup : addr_table_lookups) {
    const auto& indexes = account.is_writable
                              ? addr_table_lookup.write_indexes()
                              : addr_table_lookup.read_indexes();
    if (account.pubkey != addr_table_lookup.account_key().ToBase58()) {
      index_of_combined_array += indexes.size();
      continue;
    }

    for (size_t i = 0; i < indexes.size(); ++i) {
      CHECK(account.address_table_lookup_index.has_value());
      if (*account.address_table_lookup_index == indexes[i]) {
        return index_of_combined_array + i;
      }
    }
  }

  return std::nullopt;
}

}  // namespace

SolanaCompiledInstruction::SolanaCompiledInstruction(
    uint8_t program_id_index,
    const std::vector<uint8_t>& account_indexes,
    const std::vector<uint8_t>& data)
    : program_id_index_(program_id_index),
      account_indexes_(account_indexes),
      data_(data) {}

SolanaCompiledInstruction::SolanaCompiledInstruction(
    SolanaCompiledInstruction&&) = default;
SolanaCompiledInstruction& SolanaCompiledInstruction::operator=(
    SolanaCompiledInstruction&&) = default;
SolanaCompiledInstruction::~SolanaCompiledInstruction() = default;

bool SolanaCompiledInstruction::operator==(
    const SolanaCompiledInstruction& ins) const {
  return program_id_index_ == ins.program_id_index_ &&
         account_indexes_ == ins.account_indexes_ && data_ == ins.data_;
}

// static
std::optional<SolanaCompiledInstruction>
SolanaCompiledInstruction::FromInstruction(
    const SolanaInstruction& instruction,
    const std::vector<SolanaAddress>& static_accounts,
    const std::vector<SolanaMessageAddressTableLookup>& addr_table_lookups,
    uint8_t num_of_total_write_indexes) {
  // Program ID must come from static accounts.
  // https://docs.rs/solana-program/1.14.12/src/solana_program/message/versions/v0/mod.rs.html#72-73
  auto program_id_index =
      FindIndexInStaticAccounts(static_accounts, instruction.GetProgramId());
  if (!program_id_index) {
    return std::nullopt;
  }

  std::vector<uint8_t> account_indexes;
  for (const auto& account : instruction.GetAccounts()) {
    std::optional<uint8_t> account_index;
    if (!account.address_table_lookup_index) {  // static accounts
      account_index =
          FindIndexInStaticAccounts(static_accounts, account.pubkey);
      if (!account_index) {
        return std::nullopt;
      }
    } else {  // dynamic loaded accounts
      account_index = FindIndexInAddressTableLookups(
          addr_table_lookups, account, static_accounts.size(),
          num_of_total_write_indexes);
      if (!account_index) {
        return std::nullopt;
      }
    }
    account_indexes.emplace_back(*account_index);
  }

  return SolanaCompiledInstruction(*program_id_index, account_indexes,
                                   instruction.data());
}

void SolanaCompiledInstruction::Serialize(std::vector<uint8_t>* bytes) const {
  DCHECK(bytes);

  bytes->emplace_back(program_id_index_);

  CompactU16Encode(account_indexes_.size(), bytes);
  bytes->insert(bytes->end(), account_indexes_.begin(), account_indexes_.end());

  CompactU16Encode(data_.size(), bytes);
  bytes->insert(bytes->end(), data_.begin(), data_.end());
}

// static
std::optional<SolanaCompiledInstruction> SolanaCompiledInstruction::Deserialize(
    const std::vector<uint8_t>& bytes,
    size_t* bytes_index) {
  DCHECK(bytes_index);

  if (*bytes_index >= bytes.size()) {
    return std::nullopt;
  }
  uint8_t program_id_index = bytes[(*bytes_index)++];

  auto account_indexes = CompactArrayDecode(bytes, bytes_index);
  auto data = CompactArrayDecode(bytes, bytes_index);
  if (!account_indexes || !data) {
    return std::nullopt;
  }

  return SolanaCompiledInstruction(program_id_index, *account_indexes, *data);
}

}  // namespace brave_wallet

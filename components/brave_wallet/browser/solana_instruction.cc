/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction.h"

#include <limits>

#include "base/check.h"
#include "brave/components/brave_wallet/browser/solana_utils.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

namespace {

absl::optional<uint8_t> GetAccountIndex(
    const std::vector<SolanaAccountMeta>& accounts,
    const std::string& target_account) {
  if (accounts.size() > std::numeric_limits<uint8_t>::max())
    return absl::nullopt;

  auto it = std::find_if(
      accounts.begin(), accounts.end(),
      [&](const auto& account) { return account.pubkey == target_account; });

  if (it == accounts.end())
    return absl::nullopt;

  return it - accounts.begin();
}

}  // namespace

SolanaInstruction::SolanaInstruction(
    const std::string& program_id,
    const std::vector<SolanaAccountMeta>& accounts,
    const std::vector<uint8_t>& data)
    : program_id_(program_id), accounts_(accounts), data_(data) {}

SolanaInstruction::SolanaInstruction(const SolanaInstruction&) = default;

SolanaInstruction::~SolanaInstruction() = default;

bool SolanaInstruction::operator==(const SolanaInstruction& ins) const {
  return program_id_ == ins.program_id_ && accounts_ == ins.accounts_ &&
         data_ == ins.data_;
}

// An instruction contains a program id index, followed by a compact-array of
// account address indexes, followed by a compact-array of opaque 8-bit data.
// The program id index is used to identify an on-chain program that can
// interpret the opaque data. The program id index is an unsigned 8-bit index to
// an account address in the message's array of account addresses. The account
// address indexes are each an unsigned 8-bit index into that same array.
bool SolanaInstruction::Serialize(
    const std::vector<SolanaAccountMeta>& message_account_metas,
    std::vector<uint8_t>* bytes) const {
  DCHECK(bytes);

  if (message_account_metas.size() > std::numeric_limits<uint8_t>::max() ||
      accounts_.size() > std::numeric_limits<uint8_t>::max() ||
      accounts_.size() > message_account_metas.size())
    return false;

  // Program ID index.
  auto program_id_index = GetAccountIndex(message_account_metas, program_id_);
  if (!program_id_index)
    return false;
  bytes->push_back(program_id_index.value());

  // A compact array of account address indexes.
  CompactU16Encode(accounts_.size(), bytes);
  for (const auto& account : accounts_) {
    auto account_index = GetAccountIndex(message_account_metas, account.pubkey);
    if (!account_index)
      return false;
    bytes->push_back(account_index.value());
  }

  // A compact array of instruction data.
  CompactU16Encode(data_.size(), bytes);
  bytes->insert(bytes->end(), data_.begin(), data_.end());
  return true;
}

}  // namespace brave_wallet

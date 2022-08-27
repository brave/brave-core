/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction.h"

#include <limits>
#include <tuple>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/solana_utils.h"

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

SolanaInstruction::SolanaInstruction(const std::string& program_id,
                                     std::vector<SolanaAccountMeta>&& accounts,
                                     const std::vector<uint8_t>& data)
    : program_id_(program_id), accounts_(std::move(accounts)), data_(data) {}

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

// static
absl::optional<SolanaInstruction> SolanaInstruction::Deserialize(
    const std::vector<SolanaAccountMeta>& message_account_metas,
    const std::vector<uint8_t>& bytes,
    size_t* bytes_index) {
  DCHECK(bytes_index);
  if (*bytes_index >= bytes.size())
    return absl::nullopt;

  uint8_t program_id_index = bytes[(*bytes_index)++];
  if (program_id_index >= message_account_metas.size())
    return absl::nullopt;
  const std::string program_id = message_account_metas[program_id_index].pubkey;

  auto ret = CompactU16Decode(bytes, *bytes_index);
  if (!ret)
    return absl::nullopt;
  const uint16_t num_of_accounts = std::get<0>(*ret);
  *bytes_index += std::get<1>(*ret);

  std::vector<SolanaAccountMeta> account_metas;
  for (size_t i = 0; i < num_of_accounts; ++i) {
    if (*bytes_index >= bytes.size())
      return absl::nullopt;
    uint8_t account_index = bytes[(*bytes_index)++];
    if (account_index >= message_account_metas.size())
      return absl::nullopt;
    account_metas.emplace_back(message_account_metas[account_index]);
  }

  ret = CompactU16Decode(bytes, *bytes_index);
  if (!ret)
    return absl::nullopt;
  const uint16_t num_of_data_bytes = std::get<0>(*ret);
  *bytes_index += std::get<1>(*ret);
  if (*bytes_index + num_of_data_bytes > bytes.size())
    return absl::nullopt;
  std::vector<uint8_t> data(bytes.begin() + *bytes_index,
                            bytes.begin() + *bytes_index + num_of_data_bytes);
  *bytes_index += num_of_data_bytes;

  return SolanaInstruction(program_id, std::move(account_metas), data);
}

mojom::SolanaInstructionPtr SolanaInstruction::ToMojomSolanaInstruction()
    const {
  std::vector<mojom::SolanaAccountMetaPtr> mojom_account_metas;
  for (const auto& account : accounts_)
    mojom_account_metas.push_back(account.ToMojomSolanaAccountMeta());
  return mojom::SolanaInstruction::New(program_id_,
                                       std::move(mojom_account_metas), data_);
}

base::Value::Dict SolanaInstruction::ToValue() const {
  base::Value::Dict dict;
  dict.Set("program_id", program_id_);

  base::Value::List account_list;
  for (const auto& account : accounts_)
    account_list.Append(account.ToValue());
  dict.Set("accounts", std::move(account_list));
  dict.Set("data", base::Base64Encode(data_));

  return dict;
}

// static
absl::optional<SolanaInstruction> SolanaInstruction::FromValue(
    const base::Value::Dict& value) {
  const std::string* program_id = value.FindString("program_id");
  if (!program_id)
    return absl::nullopt;

  const base::Value::List* account_list = value.FindList("accounts");
  if (!account_list)
    return absl::nullopt;
  std::vector<SolanaAccountMeta> accounts;
  for (const auto& account_value : *account_list) {
    if (!account_value.is_dict())
      return absl::nullopt;
    absl::optional<SolanaAccountMeta> account =
        SolanaAccountMeta::FromValue(account_value.GetDict());
    if (!account)
      return absl::nullopt;
    accounts.push_back(*account);
  }

  const std::string* data_base64_encoded = value.FindString("data");
  if (!data_base64_encoded)
    return absl::nullopt;
  std::string data_decoded;
  if (!base::Base64Decode(*data_base64_encoded, &data_decoded))
    return absl::nullopt;
  std::vector<uint8_t> data(data_decoded.begin(), data_decoded.end());

  return SolanaInstruction(*program_id, std::move(accounts), data);
}

// static
void SolanaInstruction::FromMojomSolanaInstructions(
    const std::vector<mojom::SolanaInstructionPtr>& mojom_instructions,
    std::vector<SolanaInstruction>* instructions) {
  if (!instructions)
    return;
  instructions->clear();
  for (const auto& mojom_instruction : mojom_instructions) {
    std::vector<SolanaAccountMeta> account_metas;
    SolanaAccountMeta::FromMojomSolanaAccountMetas(
        mojom_instruction->account_metas, &account_metas);
    instructions->push_back(SolanaInstruction(mojom_instruction->program_id,
                                              std::move(account_metas),
                                              mojom_instruction->data));
  }
}

}  // namespace brave_wallet

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_message.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/solana_utils.h"

namespace brave_wallet {

SolanaMessage::SolanaMessage(const std::string& recent_blockhash,
                             const std::string& fee_payer,
                             std::vector<SolanaInstruction>&& instructions)
    : recent_blockhash_(recent_blockhash),
      fee_payer_(fee_payer),
      instructions_(std::move(instructions)) {}

SolanaMessage::SolanaMessage(const SolanaMessage&) = default;

SolanaMessage::~SolanaMessage() = default;

bool SolanaMessage::operator==(const SolanaMessage& message) const {
  return recent_blockhash_ == message.recent_blockhash_ &&
         fee_payer_ == message.fee_payer_ &&
         instructions_ == message.instructions_;
}

// Process instructions to return an unique account meta array with following
// properties.
// 1. No duplication (each pubkey will only have one item).
// 2. Order by signer-read-write, signer-readonly, non-signer-read-write,
// non-signer-readonly.
// 3. Fee payer will always be placed as the first item.
// 4. Program IDs will be put at last.
void SolanaMessage::GetUniqueAccountMetas(
    std::vector<SolanaAccountMeta>* unique_account_metas) const {
  DCHECK(unique_account_metas);
  unique_account_metas->clear();

  std::vector<SolanaAccountMeta> account_metas;
  // Get accounts from each instruction.
  for (const auto& instruction : instructions_) {
    account_metas.insert(account_metas.end(), instruction.GetAccounts().begin(),
                         instruction.GetAccounts().end());
  }

  // Sort accounts by is_signer first, then writable.
  // The sorted array will have 4 parts as following sequence.
  // 1. Accounts need to sign and are writable.
  // 2. Accounts need to sign and read-only.
  // 3. Accounts do not need to sign and are writable.
  // 4. Accounts do not need to sign and read-only.
  std::stable_sort(account_metas.begin(), account_metas.end(),
                   [](SolanaAccountMeta a, SolanaAccountMeta b) {
                     if (a.is_signer != b.is_signer)
                       return a.is_signer;
                     if (a.is_writable != b.is_writable)
                       return a.is_writable;
                     return false;
                   });

  // Get program ID from each instruction and put at the end.
  for (const auto& instruction : instructions_) {
    account_metas.push_back(SolanaAccountMeta(instruction.GetProgramId(),
                                              false /* is_signer */,
                                              false /* is_writable */));
  }

  // Fee payer will always be placed at first.
  unique_account_metas->push_back(SolanaAccountMeta(
      fee_payer_, true /* is_signer */, true /* is_writable */));

  // Remove duplicate accounts. is_writable is updated if later account meta
  // with the same pubkey is writable.
  for (const auto& account_meta : account_metas) {
    auto it =
        std::find_if(unique_account_metas->begin(), unique_account_metas->end(),
                     [&](const SolanaAccountMeta& unique_account_meta) {
                       return unique_account_meta.pubkey == account_meta.pubkey;
                     });

    if (it == unique_account_metas->end())
      unique_account_metas->push_back(account_meta);
    else
      it->is_writable |= account_meta.is_writable;
  }
}

// A message contains a header, followed by a compact-array of account
// addresses, followed by a recent blockhash, followed by a compact-array of
// instructions.
// See
// https://docs.solana.com/developing/programming-model/transactions#message-format
// for details.
absl::optional<std::vector<uint8_t>> SolanaMessage::Serialize(
    std::vector<std::string>* signers) const {
  if (recent_blockhash_.empty() || instructions_.empty() || fee_payer_.empty())
    return absl::nullopt;

  if (signers)
    signers->clear();

  std::vector<uint8_t> message_bytes;
  std::vector<SolanaAccountMeta> unique_account_metas;
  GetUniqueAccountMetas(&unique_account_metas);

  // Message header.
  uint8_t num_required_signatures = 0;
  uint8_t num_readonly_signed_accounts = 0;
  uint8_t num_readonly_unsigned_accounts = 0;
  for (const auto& account_meta : unique_account_metas) {
    if (account_meta.is_signer) {
      if (signers)
        signers->push_back(account_meta.pubkey);
      num_required_signatures++;
      if (!account_meta.is_writable)
        num_readonly_signed_accounts++;
    } else if (!account_meta.is_writable) {
      num_readonly_unsigned_accounts++;
    }
  }
  message_bytes.push_back(num_required_signatures);
  message_bytes.push_back(num_readonly_signed_accounts);
  message_bytes.push_back(num_readonly_unsigned_accounts);

  // Compact array of account addresses.
  CompactU16Encode(unique_account_metas.size(), &message_bytes);
  for (const auto& account_meta : unique_account_metas) {
    std::vector<uint8_t> pubkey(kSolanaPubkeySize);
    if (!Base58Decode(account_meta.pubkey, &pubkey, pubkey.size()))
      return absl::nullopt;
    message_bytes.insert(message_bytes.end(), pubkey.begin(), pubkey.end());
  }

  // Recent blockhash.
  std::vector<uint8_t> recent_blockhash_bytes(kSolanaBlockhashSize);
  if (!Base58Decode(recent_blockhash_, &recent_blockhash_bytes,
                    recent_blockhash_bytes.size()))
    return absl::nullopt;
  message_bytes.insert(message_bytes.end(), recent_blockhash_bytes.begin(),
                       recent_blockhash_bytes.end());

  // Compact array of instructions.
  CompactU16Encode(instructions_.size(), &message_bytes);
  for (const auto& instruction : instructions_) {
    if (!instruction.Serialize(unique_account_metas, &message_bytes))
      return absl::nullopt;
  }

  return message_bytes;
}

mojom::SolanaTxDataPtr SolanaMessage::ToSolanaTxData() const {
  std::vector<mojom::SolanaInstructionPtr> mojom_instructions;
  for (const auto& instruction : instructions_)
    mojom_instructions.push_back(instruction.ToMojomSolanaInstruction());

  return mojom::SolanaTxData::New(recent_blockhash_, fee_payer_,
                                  std::move(mojom_instructions));
}

base::Value SolanaMessage::ToValue() const {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("recent_blockhash", recent_blockhash_);
  dict.SetStringKey("fee_payer", fee_payer_);

  base::Value instruction_list(base::Value::Type::LIST);
  for (const auto& instruction : instructions_)
    instruction_list.Append(instruction.ToValue());
  dict.SetKey("instructions", std::move(instruction_list));

  return dict;
}

// static
absl::optional<SolanaMessage> SolanaMessage::FromValue(
    const base::Value& value) {
  if (!value.is_dict())
    return absl::nullopt;

  const std::string* recent_blockhash = value.FindStringKey("recent_blockhash");
  if (!recent_blockhash)
    return absl::nullopt;

  const std::string* fee_payer = value.FindStringKey("fee_payer");
  if (!fee_payer)
    return absl::nullopt;

  const base::Value* instruction_list = value.FindKey("instructions");
  if (!instruction_list || !instruction_list->is_list())
    return absl::nullopt;
  std::vector<SolanaInstruction> instructions;
  for (const auto& instruction_value : instruction_list->GetList()) {
    absl::optional<SolanaInstruction> instruction =
        SolanaInstruction::FromValue(instruction_value);
    if (!instruction)
      return absl::nullopt;
    instructions.push_back(*instruction);
  }

  return SolanaMessage(*recent_blockhash, *fee_payer, std::move(instructions));
}

}  // namespace brave_wallet

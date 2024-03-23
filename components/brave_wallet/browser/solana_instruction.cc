/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction.h"

#include <limits>
#include <optional>
#include <tuple>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/span.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/solana_compiled_instruction.h"
#include "brave/components/brave_wallet/browser/solana_instruction_data_decoder.h"
#include "brave/components/brave_wallet/browser/solana_message_address_table_lookup.h"
#include "brave/components/brave_wallet/browser/solana_message_header.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/solana_address.h"
#include "brave/components/brave_wallet/common/solana_utils.h"

namespace {

constexpr char kProgramId[] = "program_id";
constexpr char kAccounts[] = "accounts";
constexpr char kData[] = "data";
constexpr char kDecodedData[] = "decoded_data";

}  // namespace

namespace brave_wallet {

SolanaInstruction::SolanaInstruction(const std::string& program_id,
                                     std::vector<SolanaAccountMeta>&& accounts,
                                     base::span<const uint8_t> data)
    : program_id_(program_id),
      accounts_(std::move(accounts)),
      data_(data.begin(), data.end()),
      decoded_data_(solana_ins_data_decoder::Decode(data, program_id)) {}

SolanaInstruction::SolanaInstruction(
    const std::string& program_id,
    std::vector<SolanaAccountMeta>&& accounts,
    base::span<const uint8_t> data,
    std::optional<SolanaInstructionDecodedData> decoded_data)
    : program_id_(program_id),
      accounts_(std::move(accounts)),
      data_(data.begin(), data.end()),
      decoded_data_(std::move(decoded_data)) {}

SolanaInstruction::SolanaInstruction(const SolanaInstruction&) = default;
SolanaInstruction& SolanaInstruction::operator=(const SolanaInstruction&) =
    default;
SolanaInstruction::SolanaInstruction(SolanaInstruction&&) = default;
SolanaInstruction& SolanaInstruction::operator=(SolanaInstruction&&) = default;

SolanaInstruction::~SolanaInstruction() = default;

bool SolanaInstruction::operator==(const SolanaInstruction& ins) const {
  return program_id_ == ins.program_id_ && accounts_ == ins.accounts_ &&
         data_ == ins.data_ && decoded_data_ == ins.decoded_data_;
}

// FromCompiledInstruction converts a SolanaCompiledInstruction to an
// SolanaInstruction.
//
// compiled_instruction.program_id_index_ contains an index ix pointing to an
// an entry in the static_accounts, get the address by accessing
// static_accounts[ix].
//
// compiled_instruction.account_indexes_ contains indexes pointing to an entry
// in this combined array:
// [array of static account key indexes, array of write indexes in address
// table lookups, array of read indexes in address table lookups].
//
// Case 1) account_index points to a static account.
// It's a static account if account_index ix falls in the array of static
// account key indexes. Use static_accounts[ix] to get the actual address, and
// use the message_header to find out its is_signer, is_writable properties.
//
// Case 2) account_index points to a dynamic account lives in an address lookup
// table.
// is_signer is false since they should all be static accounts. Determine
// is_writable properties based on if account_index ix falls in the array of
// write indexes or array of read indexes. Then check each table's read or
// write indexes array to see if ix falls in this table's array to get the
// address of this table and the index pointing to the address in this table.
// Signers may not be loaded through an address lookup table as documented in
// https://docs.solana.com/proposals/versioned-transactions#limitations.
//
// static
std::optional<SolanaInstruction> SolanaInstruction::FromCompiledInstruction(
    const SolanaCompiledInstruction& compiled_instruction,
    const SolanaMessageHeader& message_header,
    const std::vector<SolanaAddress>& static_accounts,
    const std::vector<SolanaMessageAddressTableLookup>& addr_table_lookups,
    uint8_t num_of_write_indexes,
    uint8_t num_of_read_indexes) {
  int num_writable_signed_accounts =
      message_header.num_required_signatures -
      message_header.num_readonly_signed_accounts;
  int num_writable_unsigned_accounts =
      static_accounts.size() - message_header.num_required_signatures -
      message_header.num_readonly_signed_accounts;
  if (num_writable_signed_accounts < 0 ||
      num_writable_unsigned_accounts < 0) {  // invalid message header
    return std::nullopt;
  }

  // Program ID of compiled_instruction should be in static accounts.
  // https://docs.rs/solana-program/1.14.12/src/solana_program/message/versions/v0/mod.rs.html#72-73
  if (compiled_instruction.program_id_index() >= static_accounts.size()) {
    return std::nullopt;
  }
  std::string program_id =
      static_accounts[compiled_instruction.program_id_index()].ToBase58();

  std::vector<SolanaAccountMeta> account_metas;
  for (const auto& account_index : compiled_instruction.account_indexes()) {
    std::string account_key;
    std::optional<uint8_t> address_table_lookup_index = std::nullopt;
    bool is_signer = false;
    bool is_writable = false;

    if (account_index < static_accounts.size()) {  // static accounts
      account_key = static_accounts[account_index].ToBase58();
      is_signer = account_index < message_header.num_required_signatures;
      is_writable =
          is_signer ? account_index < num_writable_signed_accounts
                    : account_index - message_header.num_required_signatures <
                          num_writable_unsigned_accounts;
    } else if (account_index <
               (static_accounts.size() + num_of_write_indexes +
                num_of_read_indexes)) {  // dynamic loaded accounts
      // Keep is_signer as false as all signers should be in static accounts.
      // https://docs.solana.com/proposals/versioned-transactions#limitations.
      is_writable =
          account_index < (static_accounts.size() + num_of_write_indexes);
      uint8_t start_index = is_writable
                                ? static_accounts.size()
                                : static_accounts.size() + num_of_write_indexes;
      for (const auto& addr_table_lookup : addr_table_lookups) {
        const auto& indexes = is_writable ? addr_table_lookup.write_indexes()
                                          : addr_table_lookup.read_indexes();
        if (indexes.empty()) {
          continue;
        }
        if (account_index >= start_index &&
            account_index < start_index + indexes.size()) {
          address_table_lookup_index = indexes[account_index - start_index];
          account_key = addr_table_lookup.account_key().ToBase58();
        }
        start_index += indexes.size();
      }

      if (!address_table_lookup_index) {  // not found in table lookups
        return std::nullopt;
      }
    } else {  // out of bound
      return std::nullopt;
    }
    account_metas.emplace_back(SolanaAccountMeta(
        account_key, address_table_lookup_index, is_signer, is_writable));
  }

  return SolanaInstruction(program_id, std::move(account_metas),
                           compiled_instruction.data());
}

mojom::SolanaInstructionPtr SolanaInstruction::ToMojomSolanaInstruction()
    const {
  std::vector<mojom::SolanaAccountMetaPtr> mojom_account_metas;
  for (const auto& account : accounts_) {
    mojom_account_metas.push_back(account.ToMojomSolanaAccountMeta());
  }
  mojom::DecodedSolanaInstructionDataPtr mojom_decoded_data = nullptr;
  if (decoded_data_) {
    mojom_decoded_data = decoded_data_->ToMojom();
  }

  return mojom::SolanaInstruction::New(program_id_,
                                       std::move(mojom_account_metas), data_,
                                       std::move(mojom_decoded_data));
}

base::Value::Dict SolanaInstruction::ToValue() const {
  base::Value::Dict dict;
  dict.Set(kProgramId, program_id_);

  base::Value::List account_list;
  for (const auto& account : accounts_) {
    account_list.Append(account.ToValue());
  }
  dict.Set(kAccounts, std::move(account_list));
  dict.Set(kData, base::Base64Encode(data_));

  if (decoded_data_) {
    auto decoded_data_dict = decoded_data_->ToValue();
    if (decoded_data_dict) {
      dict.Set(kDecodedData, std::move(*decoded_data_dict));
    }
  }

  return dict;
}

// static
std::optional<SolanaInstruction> SolanaInstruction::FromValue(
    const base::Value::Dict& value) {
  const std::string* program_id = value.FindString(kProgramId);
  if (!program_id) {
    return std::nullopt;
  }

  const base::Value::List* account_list = value.FindList(kAccounts);
  if (!account_list) {
    return std::nullopt;
  }
  std::vector<SolanaAccountMeta> accounts;
  for (const auto& account_value : *account_list) {
    if (!account_value.is_dict()) {
      return std::nullopt;
    }
    std::optional<SolanaAccountMeta> account =
        SolanaAccountMeta::FromValue(account_value.GetDict());
    if (!account) {
      return std::nullopt;
    }
    accounts.push_back(*account);
  }

  const std::string* data_base64_encoded = value.FindString(kData);
  if (!data_base64_encoded) {
    return std::nullopt;
  }
  std::string data_decoded;
  if (!base::Base64Decode(*data_base64_encoded, &data_decoded)) {
    return std::nullopt;
  }
  std::vector<uint8_t> data(data_decoded.begin(), data_decoded.end());

  std::optional<SolanaInstructionDecodedData> decoded_data = std::nullopt;
  const base::Value::Dict* decoded_data_dict = value.FindDict(kDecodedData);
  if (decoded_data_dict) {
    decoded_data = SolanaInstructionDecodedData::FromValue(*decoded_data_dict);
  }

  return SolanaInstruction(*program_id, std::move(accounts), data,
                           std::move(decoded_data));
}

// static
void SolanaInstruction::FromMojomSolanaInstructions(
    const std::vector<mojom::SolanaInstructionPtr>& mojom_instructions,
    std::vector<SolanaInstruction>* instructions) {
  if (!instructions) {
    return;
  }
  instructions->clear();
  for (const auto& mojom_instruction : mojom_instructions) {
    std::vector<SolanaAccountMeta> account_metas;
    SolanaAccountMeta::FromMojomSolanaAccountMetas(
        mojom_instruction->account_metas, &account_metas);
    instructions->emplace_back(SolanaInstruction(
        mojom_instruction->program_id, std::move(account_metas),
        mojom_instruction->data,
        SolanaInstructionDecodedData::FromMojom(
            mojom_instruction->program_id, mojom_instruction->decoded_data)));
  }
}

}  // namespace brave_wallet

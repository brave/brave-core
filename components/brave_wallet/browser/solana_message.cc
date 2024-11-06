/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_message.h"

#include <algorithm>
#include <optional>
#include <tuple>
#include <utility>

#include "base/check.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/solana_compiled_instruction.h"
#include "brave/components/brave_wallet/browser/solana_instruction_builder.h"
#include "brave/components/brave_wallet/browser/solana_instruction_data_decoder.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"

namespace brave_wallet {

namespace {

constexpr uint8_t kV0MessagePrefix = 0x80;
constexpr uint8_t kVersionPrefixMask = 0x7f;
constexpr char kVersion[] = "version";
constexpr char kRecentBlockhash[] = "recent_blockhash";
constexpr char kLastValidBlockHeight[] = "last_valid_block_height";
constexpr char kFeePayer[] = "fee_payer";
constexpr char kMessageHeader[] = "message_header";
constexpr char kStaticAccountKeys[] = "static_account_keys";
constexpr char kInstructions[] = "instructions";
constexpr char kAddressTableLookups[] = "address_table_lookups";

bool MaybeAddVersionPrefix(mojom::SolanaMessageVersion version,
                           std::vector<uint8_t>* message_bytes) {
  DCHECK(message_bytes);

  if (version == mojom::SolanaMessageVersion::kV0) {
    message_bytes->emplace_back(kV0MessagePrefix);
  } else if (version == mojom::SolanaMessageVersion::kLegacy) {
    // Do nothing.
  } else {  // Version not supported yet.
    return false;
  }

  return true;
}

// Deserialize message header from a serialized message.
// Returns the bytes_index after message header part, deserialized version and
// message header.
std::optional<
    std::tuple<size_t, mojom::SolanaMessageVersion, SolanaMessageHeader>>
DeserializeMessageHeader(const std::vector<uint8_t>& bytes) {
  // Check version
  if (bytes.size() < 1) {
    return std::nullopt;
  }
  mojom::SolanaMessageVersion version;
  if (bytes[0] == (bytes[0] & kVersionPrefixMask)) {
    version = mojom::SolanaMessageVersion::kLegacy;
  } else if (bytes[0] == kV0MessagePrefix) {
    version = mojom::SolanaMessageVersion::kV0;
  } else {
    return std::nullopt;
  }

  size_t bytes_index = version == mojom::SolanaMessageVersion::kLegacy ? 0 : 1;
  if (bytes_index + 3 > bytes.size()) {  // Message header length.
    return std::nullopt;
  }

  // Message header
  SolanaMessageHeader message_header;
  message_header.num_required_signatures = bytes[bytes_index++];
  message_header.num_readonly_signed_accounts = bytes[bytes_index++];
  message_header.num_readonly_unsigned_accounts = bytes[bytes_index++];

  return std::make_tuple(bytes_index, version, message_header);
}

std::optional<std::vector<SolanaMessageAddressTableLookup>>
DeserializeAddressTableLookups(const std::vector<uint8_t>& bytes,
                               size_t* bytes_index) {
  DCHECK(bytes_index);

  auto ret = CompactU16Decode(bytes, *bytes_index);
  if (!ret) {
    return std::nullopt;
  }
  *bytes_index += std::get<1>(*ret);
  size_t num_of_addr_table_lookups = std::get<0>(*ret);

  std::vector<SolanaMessageAddressTableLookup> addr_table_lookups;
  for (size_t i = 0; i < num_of_addr_table_lookups; i++) {
    auto addr_table_lookup =
        SolanaMessageAddressTableLookup::Deserialize(bytes, bytes_index);
    if (!addr_table_lookup) {
      return std::nullopt;
    }
    addr_table_lookups.emplace_back(std::move(*addr_table_lookup));
  }

  return addr_table_lookups;
}

}  // namespace

SolanaMessage::SolanaMessage(
    mojom::SolanaMessageVersion version,
    const std::string& recent_blockhash,
    uint64_t last_valid_block_height,
    const std::string& fee_payer,
    const SolanaMessageHeader& message_header,
    std::vector<SolanaAddress>&& static_account_keys,
    std::vector<SolanaInstruction>&& instructions,
    std::vector<SolanaMessageAddressTableLookup>&& address_table_lookups)
    : version_(version),
      recent_blockhash_(recent_blockhash),
      last_valid_block_height_(last_valid_block_height),
      fee_payer_(fee_payer),
      message_header_(message_header),
      static_account_keys_(std::move(static_account_keys)),
      instructions_(std::move(instructions)),
      address_table_lookups_(std::move(address_table_lookups)) {}

// static
std::optional<SolanaMessage> SolanaMessage::CreateLegacyMessage(
    const std::string& recent_blockhash,
    uint64_t last_valid_block_height,
    const std::string& fee_payer,
    std::vector<SolanaInstruction>&& instructions) {
  uint16_t num_required_signatures = 0;
  uint16_t num_readonly_signed_accounts = 0;
  uint16_t num_readonly_unsigned_accounts = 0;
  std::vector<SolanaAccountMeta> unique_account_metas;
  GetUniqueAccountMetas(fee_payer, instructions, &unique_account_metas);
  std::vector<SolanaAddress> static_accounts;

  // Check for non-legacy meta
  for (const auto& meta : unique_account_metas) {
    if (meta.address_table_lookup_index) {
      return std::nullopt;
    }
  }

  if (!ProcessAccountMetas(
          unique_account_metas, static_accounts, num_required_signatures,
          num_readonly_signed_accounts, num_readonly_unsigned_accounts)) {
    return std::nullopt;
  }

  return SolanaMessage(
      mojom::SolanaMessageVersion::kLegacy, recent_blockhash,
      last_valid_block_height, fee_payer,
      SolanaMessageHeader(num_required_signatures, num_readonly_signed_accounts,
                          num_readonly_unsigned_accounts),
      std::move(static_accounts), std::move(instructions), {});
}

SolanaMessage::SolanaMessage(SolanaMessage&&) = default;
SolanaMessage& SolanaMessage::operator=(SolanaMessage&&) = default;

SolanaMessage::~SolanaMessage() = default;

bool SolanaMessage::operator==(const SolanaMessage& message) const {
  return version_ == message.version_ &&
         recent_blockhash_ == message.recent_blockhash_ &&
         last_valid_block_height_ == message.last_valid_block_height_ &&
         fee_payer_ == message.fee_payer_ &&
         message_header_ == message.message_header_ &&
         static_account_keys_ == message.static_account_keys_ &&
         instructions_ == message.instructions_ &&
         address_table_lookups_ == message.address_table_lookups_;
}

// Process instructions to return an unique account meta array with following
// properties.
// 1. No duplication (each pubkey will only have one item).
// 2. Order by signer-read-write, signer-readonly, non-signer-read-write,
// non-signer-readonly.
// 3. Fee payer will always be placed as the first item.
// This function is currently only used when creating legacy messages and does
// not support address table lookups.
// static
void SolanaMessage::GetUniqueAccountMetas(
    const std::string& fee_payer,
    const std::vector<SolanaInstruction>& instructions,
    std::vector<SolanaAccountMeta>* unique_account_metas) {
  DCHECK(unique_account_metas);
  unique_account_metas->clear();

  std::vector<SolanaAccountMeta> account_metas;
  // Get accounts from each instruction.
  for (const auto& instruction : instructions) {
    account_metas.emplace_back(
        SolanaAccountMeta(instruction.GetProgramId(), std::nullopt,
                          false /* is_signer */, false /* is_writable */));
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
                     if (a.is_signer != b.is_signer) {
                       return a.is_signer;
                     }
                     if (a.is_writable != b.is_writable) {
                       return a.is_writable;
                     }
                     return false;
                   });

  // Fee payer will always be placed at first.
  unique_account_metas->push_back(SolanaAccountMeta(
      fee_payer, std::nullopt, true /* is_signer */, true /* is_writable */));

  // Remove duplicate accounts. is_writable is updated if later account meta
  // with the same pubkey is writable.
  for (const auto& account_meta : account_metas) {
    auto it = base::ranges::find(*unique_account_metas, account_meta.pubkey,
                                 &SolanaAccountMeta::pubkey);

    if (it == unique_account_metas->end()) {
      unique_account_metas->push_back(account_meta);
    } else {
      it->is_writable |= account_meta.is_writable;
    }
  }
}

// A message contains a header, followed by a compact-array of account
// addresses, followed by a recent blockhash, followed by a compact-array of
// instructions.
// See
// https://docs.solana.com/developing/programming-model/transactions#message-format
// for details.
std::optional<std::vector<uint8_t>> SolanaMessage::Serialize(
    std::vector<std::string>* signers) const {
  if (recent_blockhash_.empty() || instructions_.empty() ||
      fee_payer_.empty()) {
    return std::nullopt;
  }

  // Calculate read and write indexes size.
  uint16_t num_of_write_indexes = 0;
  uint16_t num_of_read_indexes = 0;
  for (const auto& address_table_lookup : address_table_lookups_) {
    num_of_write_indexes += address_table_lookup.write_indexes().size();
    num_of_read_indexes += address_table_lookup.read_indexes().size();
    if (num_of_write_indexes > UINT8_MAX || num_of_read_indexes > UINT8_MAX) {
      return std::nullopt;
    }
  }

  // This is the size of the combined array of static_accounts, write_indexes
  // from address table lookups, and read_indexes from address table lookups.
  // It cannot exceed UINT8_MAX as the account indexes used in transaction is
  // limited to UINT8_MAX.
  if ((static_account_keys_.size() + num_of_write_indexes +
       num_of_read_indexes) > UINT8_MAX) {
    return std::nullopt;
  }

  if (signers) {
    signers->clear();
  }

  // Version prefix.
  std::vector<uint8_t> message_bytes;
  if (!MaybeAddVersionPrefix(version_, &message_bytes)) {
    return std::nullopt;
  }

  // Message header.
  message_bytes.emplace_back(message_header_.num_required_signatures);
  message_bytes.emplace_back(message_header_.num_readonly_signed_accounts);
  message_bytes.emplace_back(message_header_.num_readonly_unsigned_accounts);

  // Compact array of account addresses.
  CompactU16Encode(static_account_keys_.size(), &message_bytes);
  for (size_t i = 0; i < static_account_keys_.size(); ++i) {
    message_bytes.insert(message_bytes.end(),
                         static_account_keys_[i].bytes().begin(),
                         static_account_keys_[i].bytes().end());

    if (signers && i < message_header_.num_required_signatures) {
      signers->emplace_back(static_account_keys_[i].ToBase58());
    }
  }

  // Recent blockhash.
  std::vector<uint8_t> recent_blockhash_bytes(kSolanaHashSize);
  if (!Base58Decode(recent_blockhash_, &recent_blockhash_bytes,
                    recent_blockhash_bytes.size())) {
    return std::nullopt;
  }
  message_bytes.insert(message_bytes.end(), recent_blockhash_bytes.begin(),
                       recent_blockhash_bytes.end());

  // Compact array of instructions.
  CompactU16Encode(instructions_.size(), &message_bytes);
  for (const auto& instruction : instructions_) {
    auto compiled_instruction = SolanaCompiledInstruction::FromInstruction(
        instruction, static_account_keys_, address_table_lookups_,
        num_of_write_indexes);
    if (!compiled_instruction) {
      return std::nullopt;
    }
    compiled_instruction->Serialize(&message_bytes);
  }

  // Compact array of address table lookups.
  if (version_ == mojom::SolanaMessageVersion::kV0) {
    CompactU16Encode(address_table_lookups_.size(), &message_bytes);
    for (const auto& address_table_lookup : address_table_lookups_) {
      address_table_lookup.Serialize(&message_bytes);
    }
  }

  return message_bytes;
}

std::optional<std::vector<std::string>>
SolanaMessage::GetSignerAccountsFromSerializedMessage(
    const std::vector<uint8_t>& serialized_message) {
  auto tuple = DeserializeMessageHeader(serialized_message);
  if (!tuple) {
    return std::nullopt;
  }
  size_t index = std::get<0>(*tuple);
  SolanaMessageHeader message_header = std::get<2>(*tuple);

  // Consume length of account array.
  auto ret = CompactU16Decode(serialized_message, index);
  if (!ret) {
    return std::nullopt;
  }
  index += std::get<1>(*ret);

  std::vector<std::string> signers;
  for (size_t i = 0; i < message_header.num_required_signatures; ++i) {
    if (index + kSolanaPubkeySize > serialized_message.size()) {
      return std::nullopt;
    }

    const std::vector<uint8_t> address_bytes(
        serialized_message.begin() + index,
        serialized_message.begin() + index + kSolanaPubkeySize);
    signers.push_back(Base58Encode(address_bytes));
    index += kSolanaPubkeySize;
  }

  return signers;
}

// static
std::optional<SolanaMessage> SolanaMessage::Deserialize(
    const std::vector<uint8_t>& bytes) {
  auto tuple = DeserializeMessageHeader(bytes);
  if (!tuple) {
    return std::nullopt;
  }
  size_t bytes_index = std::get<0>(*tuple);
  mojom::SolanaMessageVersion version = std::get<1>(*tuple);
  SolanaMessageHeader message_header = std::get<2>(*tuple);

  // Compact array of account addresses
  auto ret = CompactU16Decode(bytes, bytes_index);
  if (!ret) {
    return std::nullopt;
  }
  const uint16_t num_of_accounts = std::get<0>(*ret);
  if (num_of_accounts == 0 || num_of_accounts > UINT8_MAX) {
    return std::nullopt;
  }
  bytes_index += std::get<1>(*ret);

  std::vector<SolanaAddress> accounts;
  for (size_t i = 0; i < num_of_accounts; ++i) {
    if (bytes_index + kSolanaPubkeySize > bytes.size()) {
      return std::nullopt;
    }
    auto account_key = SolanaAddress::FromBytes(
        base::make_span(bytes).subspan(bytes_index, kSolanaPubkeySize));
    if (!account_key) {
      return std::nullopt;
    }
    accounts.emplace_back(*account_key);
    bytes_index += kSolanaPubkeySize;
  }
  std::string fee_payer = accounts[0].ToBase58();

  // Blockhash
  if (bytes_index + kSolanaHashSize > bytes.size()) {
    return std::nullopt;
  }
  const std::vector<uint8_t> blockhash_bytes(
      bytes.begin() + bytes_index,
      bytes.begin() + bytes_index + kSolanaHashSize);
  bytes_index += kSolanaHashSize;
  const std::string recent_blockhash = Base58Encode(blockhash_bytes);

  // Instructions length
  ret = CompactU16Decode(bytes, bytes_index);
  if (!ret) {
    return std::nullopt;
  }
  bytes_index += std::get<1>(*ret);
  size_t num_of_instructions = std::get<0>(*ret);

  std::vector<SolanaCompiledInstruction> compiled_instructions;
  for (size_t i = 0; i < num_of_instructions; ++i) {
    auto compiled_instruction =
        SolanaCompiledInstruction::Deserialize(bytes, &bytes_index);
    if (!compiled_instruction) {
      return std::nullopt;
    }
    compiled_instructions.emplace_back(std::move(*compiled_instruction));
  }

  std::optional<std::vector<SolanaMessageAddressTableLookup>>
      addr_table_lookups = std::vector<SolanaMessageAddressTableLookup>();
  if (version == mojom::SolanaMessageVersion::kV0) {
    addr_table_lookups = DeserializeAddressTableLookups(bytes, &bytes_index);
    if (!addr_table_lookups) {
      return std::nullopt;
    }
  }

  // Byte array needs to be exact without any left over bytes.
  if (bytes_index != bytes.size()) {
    return std::nullopt;
  }

  // Size of combined array of static_accounts_array, write_indexes_array,
  // read_indexes_array cannot exceed UINT8_MAX.
  uint16_t num_of_write_indexes = 0;
  uint16_t num_of_read_indexes = 0;
  for (const auto& addr_table_lookup : *addr_table_lookups) {
    num_of_write_indexes += addr_table_lookup.write_indexes().size();
    num_of_read_indexes += addr_table_lookup.read_indexes().size();
    if (num_of_write_indexes > UINT8_MAX || num_of_read_indexes > UINT8_MAX) {
      return std::nullopt;
    }
  }
  if ((accounts.size() + num_of_write_indexes + num_of_read_indexes) >
      UINT8_MAX) {
    return std::nullopt;
  }

  // Convert compiled_instructions to instructions.
  std::vector<SolanaInstruction> instructions;
  for (const auto& compiled_instruction : compiled_instructions) {
    auto ins = SolanaInstruction::FromCompiledInstruction(
        compiled_instruction, message_header, accounts, *addr_table_lookups,
        num_of_write_indexes, num_of_read_indexes);
    if (!ins) {
      return std::nullopt;
    }
    instructions.emplace_back(std::move(*ins));
  }

  return SolanaMessage(version, recent_blockhash, 0, fee_payer, message_header,
                       std::move(accounts), std::move(instructions),
                       std::move(*addr_table_lookups));
}

mojom::SolanaTxDataPtr SolanaMessage::ToSolanaTxData() const {
  std::vector<mojom::SolanaInstructionPtr> mojom_instructions;
  for (const auto& instruction : instructions_) {
    mojom_instructions.push_back(instruction.ToMojomSolanaInstruction());
  }

  auto solana_tx_data = mojom::SolanaTxData::New();
  solana_tx_data->version = version_;
  solana_tx_data->recent_blockhash = recent_blockhash_;
  solana_tx_data->last_valid_block_height = last_valid_block_height_;
  solana_tx_data->fee_payer = fee_payer_;
  solana_tx_data->instructions = std::move(mojom_instructions);

  std::vector<std::string> base58_static_account_keys;
  for (const auto& static_account_key : static_account_keys_) {
    base58_static_account_keys.emplace_back(static_account_key.ToBase58());
  }
  solana_tx_data->static_account_keys = base58_static_account_keys;

  solana_tx_data->message_header = message_header_.ToMojom();
  solana_tx_data->address_table_lookups =
      SolanaMessageAddressTableLookup::ToMojomArray(address_table_lookups_);
  return solana_tx_data;
}

base::Value::Dict SolanaMessage::ToValue() const {
  base::Value::Dict dict;

  dict.Set(kVersion, static_cast<int32_t>(version_));
  dict.Set(kRecentBlockhash, recent_blockhash_);
  dict.Set(kLastValidBlockHeight,
           base::NumberToString(last_valid_block_height_));
  dict.Set(kFeePayer, fee_payer_);
  dict.Set(kMessageHeader, message_header_.ToValue());

  base::Value::List static_account_key_list;
  for (const auto& static_account_key : static_account_keys_) {
    static_account_key_list.Append(static_account_key.ToBase58());
  }
  dict.Set(kStaticAccountKeys, std::move(static_account_key_list));

  base::Value::List instruction_list;
  for (const auto& instruction : instructions_) {
    instruction_list.Append(instruction.ToValue());
  }
  dict.Set(kInstructions, std::move(instruction_list));

  base::Value::List address_table_lookup_list;
  for (const auto& address_table_lookup : address_table_lookups_) {
    address_table_lookup_list.Append(address_table_lookup.ToValue());
  }
  dict.Set(kAddressTableLookups, std::move(address_table_lookup_list));

  return dict;
}

// static
std::optional<SolanaMessage> SolanaMessage::FromValue(
    const base::Value::Dict& value) {
  auto version = value.FindInt(kVersion);
  if (!version || !mojom::IsKnownEnumValue(
                      static_cast<mojom::SolanaMessageVersion>(*version))) {
    return std::nullopt;
  }

  const std::string* recent_blockhash = value.FindString(kRecentBlockhash);
  if (!recent_blockhash) {
    return std::nullopt;
  }

  const std::string* last_valid_block_height_string =
      value.FindString(kLastValidBlockHeight);
  uint64_t last_valid_block_height = 0;
  if (!last_valid_block_height_string ||
      !base::StringToUint64(*last_valid_block_height_string,
                            &last_valid_block_height)) {
    return std::nullopt;
  }

  const std::string* fee_payer = value.FindString(kFeePayer);
  if (!fee_payer) {
    return std::nullopt;
  }

  const base::Value::Dict* message_header_dict = value.FindDict(kMessageHeader);
  if (!message_header_dict) {
    return std::nullopt;
  }
  auto message_header = SolanaMessageHeader::FromValue(*message_header_dict);
  if (!message_header) {
    return std::nullopt;
  }

  const base::Value::List* static_account_key_list =
      value.FindList(kStaticAccountKeys);
  if (!static_account_key_list) {
    return std::nullopt;
  }
  std::vector<SolanaAddress> static_account_keys;
  for (const auto& static_account_key_value : *static_account_key_list) {
    if (!static_account_key_value.is_string()) {
      return std::nullopt;
    }
    auto address =
        SolanaAddress::FromBase58(static_account_key_value.GetString());
    if (!address) {
      return std::nullopt;
    }
    static_account_keys.emplace_back(*address);
  }

  const base::Value::List* instruction_list = value.FindList(kInstructions);
  if (!instruction_list) {
    return std::nullopt;
  }
  std::vector<SolanaInstruction> instructions;
  for (const auto& instruction_value : *instruction_list) {
    if (!instruction_value.is_dict()) {
      return std::nullopt;
    }

    std::optional<SolanaInstruction> instruction =
        SolanaInstruction::FromValue(instruction_value.GetDict());
    if (!instruction) {
      return std::nullopt;
    }
    instructions.emplace_back(std::move(instruction.value()));
  }

  const base::Value::List* address_table_lookup_list =
      value.FindList(kAddressTableLookups);
  if (!address_table_lookup_list) {
    return std::nullopt;
  }
  std::vector<SolanaMessageAddressTableLookup> address_table_lookups;
  for (const auto& address_table_lookup_value : *address_table_lookup_list) {
    if (!address_table_lookup_value.is_dict()) {
      return std::nullopt;
    }

    std::optional<SolanaMessageAddressTableLookup> address_table_lookup =
        SolanaMessageAddressTableLookup::FromValue(
            address_table_lookup_value.GetDict());
    if (!address_table_lookup) {
      return std::nullopt;
    }
    address_table_lookups.emplace_back(std::move(address_table_lookup.value()));
  }

  return SolanaMessage(static_cast<mojom::SolanaMessageVersion>(*version),
                       *recent_blockhash, last_valid_block_height, *fee_payer,
                       *message_header, std::move(static_account_keys),
                       std::move(instructions),
                       std::move(address_table_lookups));
}

// static
std::optional<SolanaMessage> SolanaMessage::FromDeprecatedLegacyValue(
    const base::Value::Dict& value) {
  const std::string* recent_blockhash = value.FindString(kRecentBlockhash);
  if (!recent_blockhash) {
    return std::nullopt;
  }

  const std::string* last_valid_block_height_string =
      value.FindString(kLastValidBlockHeight);
  uint64_t last_valid_block_height = 0;
  if (!last_valid_block_height_string ||
      !base::StringToUint64(*last_valid_block_height_string,
                            &last_valid_block_height)) {
    return std::nullopt;
  }

  const std::string* fee_payer = value.FindString(kFeePayer);
  if (!fee_payer) {
    return std::nullopt;
  }

  const base::Value::List* instruction_list = value.FindList(kInstructions);
  if (!instruction_list) {
    return std::nullopt;
  }
  std::vector<SolanaInstruction> instructions;
  for (const auto& instruction_value : *instruction_list) {
    if (!instruction_value.is_dict()) {
      return std::nullopt;
    }

    std::optional<SolanaInstruction> instruction =
        SolanaInstruction::FromValue(instruction_value.GetDict());
    if (!instruction) {
      return std::nullopt;
    }
    instructions.emplace_back(std::move(instruction.value()));
  }

  return CreateLegacyMessage(*recent_blockhash, last_valid_block_height,
                             *fee_payer, std::move(instructions));
}

bool SolanaMessage::UsesDurableNonce() const {
  if (instructions_.empty()) {
    return false;
  }

  const auto& instruction = instructions_[0];
  if (instruction.GetAccounts().empty()) {
    return false;
  }

  const auto& account = instruction.GetAccounts()[0];

  // Is a nonce advance instruction from system program.
  if (solana_ins_data_decoder::GetSystemInstructionType(
          instruction.data(), instruction.GetProgramId()) !=
      mojom::SolanaSystemInstruction::kAdvanceNonceAccount) {
    return false;
  }

  // Nonce account is writable.
  if (!account.is_writable) {
    return false;
  }

  return true;
}

bool SolanaMessage::ContainsCompressedNftTransfer() const {
  for (const auto& instruction : instructions_) {
    if (solana_ins_data_decoder::IsCompressedNftTransferInstruction(
            instruction.data(), instruction.GetProgramId())) {
      return true;
    }
  }

  return false;
}

bool SolanaMessage::UsesPriorityFee() const {
  for (const auto& instruction : instructions_) {
    auto instruction_type =
        solana_ins_data_decoder::GetComputeBudgetInstructionType(
            instruction.data(), instruction.GetProgramId());
    if (instruction_type ==
            mojom::SolanaComputeBudgetInstruction::kSetComputeUnitPrice ||
        instruction_type ==
            mojom::SolanaComputeBudgetInstruction::kSetComputeUnitLimit) {
      return true;
    }
  }

  return false;
}

bool SolanaMessage::AddPriorityFee(uint32_t compute_units,
                                   uint64_t fee_per_compute_unit) {
  SolanaInstruction modify_compute_units_instruction =
      solana::compute_budget_program::SetComputeUnitLimit(compute_units);
  SolanaInstruction add_priority_fee_instruction =
      solana::compute_budget_program::SetComputeUnitPrice(fee_per_compute_unit);

  // Do not add a priority fee if there already is one added.
  if (UsesPriorityFee()) {
    return false;
  }

  if (UsesDurableNonce()) {
    // The first instruction should remain the advance nonce instruction.
    // https://solana.com/developers/guides/advanced/how-to-use-priority-fees#special-considerations
    instructions_.insert(
        instructions_.begin() + 1,
        {modify_compute_units_instruction, add_priority_fee_instruction});
  } else {
    instructions_.insert(
        instructions_.begin(),
        {modify_compute_units_instruction, add_priority_fee_instruction});
  }

  uint16_t num_required_signatures = 0;
  uint16_t num_readonly_signed_accounts = 0;
  uint16_t num_readonly_unsigned_accounts = 0;
  std::vector<SolanaAccountMeta> unique_account_metas;
  GetUniqueAccountMetas(fee_payer_, instructions_, &unique_account_metas);
  std::vector<SolanaAddress> static_accounts;

  if (!ProcessAccountMetas(
          unique_account_metas, static_accounts, num_required_signatures,
          num_readonly_signed_accounts, num_readonly_unsigned_accounts)) {
    return false;
  }

  static_account_keys_ = static_accounts;
  message_header_ =
      SolanaMessageHeader(num_required_signatures, num_readonly_signed_accounts,
                          num_readonly_unsigned_accounts);
  return true;
}

// static
bool SolanaMessage::ProcessAccountMetas(
    const std::vector<SolanaAccountMeta>& unique_account_metas,
    std::vector<SolanaAddress>& static_accounts,
    uint16_t& num_required_signatures,
    uint16_t& num_readonly_signed_accounts,
    uint16_t& num_readonly_unsigned_accounts) {
  static_accounts.clear();
  for (const auto& meta : unique_account_metas) {
    auto addr = SolanaAddress::FromBase58(meta.pubkey);
    if (!addr) {
      return false;
    }

    if (meta.is_signer) {
      num_required_signatures++;
    }
    if (meta.is_signer && !meta.is_writable) {
      num_readonly_signed_accounts++;
    }
    if (!meta.is_signer && !meta.is_writable) {
      num_readonly_unsigned_accounts++;
    }

    if (num_required_signatures > UINT8_MAX ||
        num_readonly_signed_accounts > UINT8_MAX ||
        num_readonly_unsigned_accounts > UINT8_MAX ||
        static_accounts.size() == UINT8_MAX) {
      return false;
    }
    static_accounts.emplace_back(*addr);
  }

  return true;
}

}  // namespace brave_wallet

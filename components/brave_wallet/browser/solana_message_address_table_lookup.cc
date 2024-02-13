/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_message_address_table_lookup.h"

#include <optional>

#include "base/base64.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/solana_utils.h"

namespace {

constexpr char kAccountKey[] = "account_key";
constexpr char kBase64EncodedWriteIndexes[] = "base64_encoded_write_indexes";
constexpr char kBase64EncodedReadIndexes[] = "base64_encoded_read_indexes";

std::optional<std::vector<uint8_t>> GetIndexesFromBase64EncodedStringDict(
    const base::Value::Dict& value,
    const std::string& dict_key) {
  const std::string* base64_encoded_indexes = value.FindString(dict_key);
  if (!base64_encoded_indexes) {
    return std::nullopt;
  }
  return base::Base64Decode(*base64_encoded_indexes);
}

}  // namespace

namespace brave_wallet {

SolanaMessageAddressTableLookup::SolanaMessageAddressTableLookup(
    const SolanaAddress& account_key,
    const std::vector<uint8_t>& write_indexes,
    const std::vector<uint8_t>& read_indexes)
    : account_key_(account_key),
      write_indexes_(write_indexes),
      read_indexes_(read_indexes) {}

SolanaMessageAddressTableLookup::SolanaMessageAddressTableLookup(
    SolanaMessageAddressTableLookup&&) = default;
SolanaMessageAddressTableLookup& SolanaMessageAddressTableLookup::operator=(
    SolanaMessageAddressTableLookup&&) = default;
SolanaMessageAddressTableLookup::~SolanaMessageAddressTableLookup() = default;

bool SolanaMessageAddressTableLookup::operator==(
    const SolanaMessageAddressTableLookup& lookup) const {
  return account_key_ == lookup.account_key_ &&
         write_indexes_ == lookup.write_indexes_ &&
         read_indexes_ == lookup.read_indexes_;
}

void SolanaMessageAddressTableLookup::Serialize(
    std::vector<uint8_t>* bytes) const {
  DCHECK(bytes);

  bytes->insert(bytes->end(), account_key_.bytes().begin(),
                account_key_.bytes().end());

  CompactU16Encode(write_indexes_.size(), bytes);
  bytes->insert(bytes->end(), write_indexes_.begin(), write_indexes_.end());

  CompactU16Encode(read_indexes_.size(), bytes);
  bytes->insert(bytes->end(), read_indexes_.begin(), read_indexes_.end());
}

// static
std::optional<SolanaMessageAddressTableLookup>
SolanaMessageAddressTableLookup::Deserialize(const std::vector<uint8_t>& bytes,
                                             size_t* bytes_index) {
  DCHECK(bytes_index);
  if (*bytes_index >= bytes.size()) {
    return std::nullopt;
  }

  // Account key.
  if (*bytes_index + kSolanaPubkeySize > bytes.size()) {
    return std::nullopt;
  }
  auto account_key = SolanaAddress::FromBytes(
      base::make_span(bytes).subspan(*bytes_index, kSolanaPubkeySize));
  if (!account_key) {
    return std::nullopt;
  }
  *bytes_index += kSolanaPubkeySize;

  auto write_indexes = CompactArrayDecode(bytes, bytes_index);
  if (!write_indexes || write_indexes->size() > UINT8_MAX) {
    return std::nullopt;
  }

  auto read_indexes = CompactArrayDecode(bytes, bytes_index);
  if (!read_indexes || read_indexes->size() > UINT8_MAX) {
    return std::nullopt;
  }

  return SolanaMessageAddressTableLookup(*account_key, *write_indexes,
                                         *read_indexes);
}

base::Value::Dict SolanaMessageAddressTableLookup::ToValue() const {
  base::Value::Dict dict;
  dict.Set(kAccountKey, account_key_.ToBase58());
  dict.Set(kBase64EncodedWriteIndexes, base::Base64Encode(write_indexes_));
  dict.Set(kBase64EncodedReadIndexes, base::Base64Encode(read_indexes_));
  return dict;
}

// static
std::optional<SolanaMessageAddressTableLookup>
SolanaMessageAddressTableLookup::FromValue(const base::Value::Dict& value) {
  const std::string* account_key_str = value.FindString(kAccountKey);
  if (!account_key_str) {
    return std::nullopt;
  }
  auto account_key = SolanaAddress::FromBase58(*account_key_str);
  if (!account_key) {
    return std::nullopt;
  }

  auto write_indexes =
      GetIndexesFromBase64EncodedStringDict(value, kBase64EncodedWriteIndexes);
  if (!write_indexes) {
    return std::nullopt;
  }

  auto read_indexes =
      GetIndexesFromBase64EncodedStringDict(value, kBase64EncodedReadIndexes);
  if (!read_indexes) {
    return std::nullopt;
  }

  return SolanaMessageAddressTableLookup(*account_key, *write_indexes,
                                         *read_indexes);
}

// static
std::vector<mojom::SolanaMessageAddressTableLookupPtr>
SolanaMessageAddressTableLookup::ToMojomArray(
    const std::vector<SolanaMessageAddressTableLookup>& address_table_lookups) {
  std::vector<mojom::SolanaMessageAddressTableLookupPtr> mojom_arr;
  for (const auto& lookup : address_table_lookups) {
    mojom_arr.emplace_back(mojom::SolanaMessageAddressTableLookup::New(
        lookup.account_key().ToBase58(), lookup.write_indexes(),
        lookup.read_indexes()));
  }
  return mojom_arr;
}

// static
std::optional<std::vector<SolanaMessageAddressTableLookup>>
SolanaMessageAddressTableLookup::FromMojomArray(
    const std::vector<mojom::SolanaMessageAddressTableLookupPtr>&
        mojom_lookups) {
  std::vector<SolanaMessageAddressTableLookup> ret_arr;
  for (const auto& mojom_lookup : mojom_lookups) {
    auto account_key = SolanaAddress::FromBase58(mojom_lookup->account_key);
    if (!account_key) {
      return std::nullopt;
    }
    ret_arr.emplace_back(SolanaMessageAddressTableLookup(
        *account_key, mojom_lookup->write_indexes, mojom_lookup->read_indexes));
  }
  return ret_arr;
}

}  // namespace brave_wallet

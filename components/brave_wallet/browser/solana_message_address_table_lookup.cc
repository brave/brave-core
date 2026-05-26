/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_message_address_table_lookup.h"

#include <optional>

#include "base/base64.h"
#include "base/containers/extend.h"
#include "base/containers/span_reader.h"
#include "brave/components/brave_wallet/common/solana_utils.h"

namespace {

constexpr char kAccountKey[] = "account_key";
constexpr char kBase64EncodedWriteIndexes[] = "base64_encoded_write_indexes";
constexpr char kBase64EncodedReadIndexes[] = "base64_encoded_read_indexes";

std::optional<std::vector<uint8_t>> GetIndexesFromBase64EncodedStringDict(
    const base::DictValue& value,
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
    SolanaAddress account_key,
    std::vector<uint8_t> write_indexes,
    std::vector<uint8_t> read_indexes)
    : account_key_(std::move(account_key)),
      write_indexes_(std::move(write_indexes)),
      read_indexes_(std::move(read_indexes)) {}

SolanaMessageAddressTableLookup::SolanaMessageAddressTableLookup(
    SolanaMessageAddressTableLookup&&) = default;
SolanaMessageAddressTableLookup& SolanaMessageAddressTableLookup::operator=(
    SolanaMessageAddressTableLookup&&) = default;
SolanaMessageAddressTableLookup::~SolanaMessageAddressTableLookup() = default;

void SolanaMessageAddressTableLookup::Serialize(
    std::vector<uint8_t>& bytes) const {
  base::Extend(bytes, account_key_.bytes());

  base::Extend(bytes, CompactU16Encode(write_indexes_.size()));
  base::Extend(bytes, write_indexes_);

  base::Extend(bytes, CompactU16Encode(read_indexes_.size()));
  base::Extend(bytes, read_indexes_);
}

// static
std::optional<SolanaMessageAddressTableLookup>
SolanaMessageAddressTableLookup::Deserialize(
    base::SpanReader<const uint8_t>& reader) {
  auto account_key = SolanaAddress::ReadFrom(reader);
  if (!account_key) {
    return std::nullopt;
  }

  auto write_indexes = CompactArrayDecode(reader);
  if (!write_indexes || write_indexes->size() > UINT8_MAX) {
    return std::nullopt;
  }

  auto read_indexes = CompactArrayDecode(reader);
  if (!read_indexes || read_indexes->size() > UINT8_MAX) {
    return std::nullopt;
  }

  return SolanaMessageAddressTableLookup(std::move(*account_key),
                                         std::move(*write_indexes),
                                         std::move(*read_indexes));
}

base::DictValue SolanaMessageAddressTableLookup::ToValue() const {
  base::DictValue dict;
  dict.Set(kAccountKey, account_key_.ToBase58());
  dict.Set(kBase64EncodedWriteIndexes, base::Base64Encode(write_indexes_));
  dict.Set(kBase64EncodedReadIndexes, base::Base64Encode(read_indexes_));
  return dict;
}

// static
std::optional<SolanaMessageAddressTableLookup>
SolanaMessageAddressTableLookup::FromValue(const base::DictValue& value) {
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

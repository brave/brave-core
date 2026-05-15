/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_assets.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>

#include "base/containers/to_vector.h"

namespace brave_wallet {

namespace {

std::string JoinStorageKeys(base::span<const std::string> storage_keys) {
  std::string joined;
  for (const auto& storage_key : storage_keys) {
    if (!joined.empty()) {
      joined.push_back('\n');
    }
    joined.append(storage_key);
  }
  return joined;
}

std::vector<uint8_t> FromRust(const rust::Vec<uint8_t>& vec) {
  return base::ToVector(vec);
}

CxxAssetMetadata ToCxxAssetMetadata(std::array<uint8_t, 16> deposit,
                                    base::span<const uint8_t> name,
                                    base::span<const uint8_t> symbol,
                                    uint8_t decimals,
                                    bool is_frozen) {
  CxxAssetMetadata metadata;
  metadata.deposit = deposit;
  metadata.name = rust::Vec<uint8_t>();
  metadata.name.reserve(name.size());
  std::ranges::copy(name, std::back_inserter(metadata.name));
  metadata.symbol = rust::Vec<uint8_t>();
  metadata.symbol.reserve(symbol.size());
  std::ranges::copy(symbol, std::back_inserter(metadata.symbol));
  metadata.decimals = decimals;
  metadata.is_frozen = is_frozen;
  return metadata;
}

std::array<uint8_t, 16> ToArray(base::span<const uint8_t, 16> deposit) {
  std::array<uint8_t, 16> result = {};
  std::ranges::copy(deposit, result.begin());
  return result;
}

}  // namespace

AssetsList::AssetsList(const AssetsList&) = default;
AssetsList::AssetsList(AssetsList&&) noexcept = default;
AssetsList::~AssetsList() = default;
AssetsList& AssetsList::operator=(const AssetsList&) = default;
AssetsList& AssetsList::operator=(AssetsList&&) noexcept = default;

std::optional<AssetsList> AssetsList::FromStorageKeys(
    base::span<const std::string> storage_keys) {
  auto result =
      parse_assets_list_from_storage_keys(JoinStorageKeys(storage_keys));
  if (!result->is_ok()) {
    return std::nullopt;
  }

  return AssetsList(base::ToVector(result->unwrap()->identifiers));
}

AssetsList::AssetsList(std::vector<uint32_t> identifiers)
    : identifiers_(std::move(identifiers)) {}

AssetMetadata::AssetMetadata(const AssetMetadata& other)
    : AssetMetadata(ToCxxAssetMetadata(ToArray(other.deposit()),
                                       other.name_,
                                       other.symbol_,
                                       other.decimals(),
                                       other.is_frozen()),
                    other.name_,
                    other.symbol_) {}

AssetMetadata::AssetMetadata(AssetMetadata&&) noexcept = default;
AssetMetadata::~AssetMetadata() = default;
AssetMetadata& AssetMetadata::operator=(const AssetMetadata& other) {
  if (this == &other) {
    return *this;
  }

  *this = AssetMetadata(other);
  return *this;
}

AssetMetadata& AssetMetadata::operator=(AssetMetadata&&) noexcept = default;

bool AssetMetadata::operator==(const AssetMetadata& other) const {
  return std::ranges::equal(deposit(), other.deposit()) &&
         name_ == other.name_ && symbol_ == other.symbol_ &&
         decimals() == other.decimals() && is_frozen() == other.is_frozen();
}

std::optional<AssetMetadata> AssetMetadata::FromBytes(
    base::span<const uint8_t> metadata_bytes) {
  auto result = parse_asset_metadata_from_scale(
      ::rust::Slice<const uint8_t>(metadata_bytes));
  if (!result->is_ok()) {
    return std::nullopt;
  }

  auto parsed = result->unwrap();
  return FromFields(parsed->deposit, FromRust(parsed->name),
                    FromRust(parsed->symbol), parsed->decimals,
                    parsed->is_frozen);
}

AssetMetadata AssetMetadata::FromFields(std::array<uint8_t, 16> deposit,
                                        std::vector<uint8_t> name,
                                        std::vector<uint8_t> symbol,
                                        uint8_t decimals,
                                        bool is_frozen) {
  auto metadata =
      ToCxxAssetMetadata(deposit, name, symbol, decimals, is_frozen);
  return AssetMetadata(std::move(metadata), std::move(name), std::move(symbol));
}

const CxxAssetMetadata& AssetMetadata::operator*() const {
  return asset_metadata_;
}

base::span<const uint8_t, 16> AssetMetadata::deposit() const {
  return asset_metadata_.deposit;
}

AssetMetadata::AssetMetadata(CxxAssetMetadata asset_metadata,
                             std::vector<uint8_t> name,
                             std::vector<uint8_t> symbol)
    : asset_metadata_(std::move(asset_metadata)),
      name_(std::move(name)),
      symbol_(std::move(symbol)) {}

}  // namespace brave_wallet

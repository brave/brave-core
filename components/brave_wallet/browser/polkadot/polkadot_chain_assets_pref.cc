/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_assets_pref.h"

#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/numerics/checked_math.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {
namespace {

constexpr char kVersionField[] = "version";
constexpr char kAssetStorageHash[] = "asset_storage_hash";
constexpr char kMetadataStorageHash[] = "metadata_storage_hash";
constexpr char kSupportedAssetIds[] = "supported_asset_ids";
constexpr char kAssetMetadata[] = "asset_metadata";
constexpr char kDeposit[] = "deposit";
constexpr char kName[] = "name";
constexpr char kSymbol[] = "symbol";
constexpr char kDecimals[] = "decimals";
constexpr char kIsFrozen[] = "is_frozen";

std::string HashToPrefString(
    const std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>& hash) {
  if (!hash) {
    return std::string();
  }
  const std::string hash_bytes(reinterpret_cast<const char*>(hash->data()),
                               hash->size());
  return "0x" + base::HexEncodeLower(hash_bytes);
}

std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> HashFromPrefString(
    const base::DictValue& dict,
    const char* key) {
  const auto* value = dict.FindString(key);
  if (!value) {
    return std::nullopt;
  }
  if (value->empty()) {
    return std::nullopt;
  }

  std::array<uint8_t, kPolkadotBlockHashSize> hash = {};
  if (!PrefixedHexStringToFixed(*value, hash)) {
    return std::nullopt;
  }
  return hash;
}

bool ReadBytes(const base::DictValue& dict,
               const char* key,
               std::vector<uint8_t>* bytes) {
  const auto* value = dict.FindString(key);
  return value && PrefixedHexStringToBytes(*value, bytes);
}

const base::DictValue* GetChainDict(const PrefService& profile_prefs,
                                    std::string_view chain_id) {
  return profile_prefs.GetDict(kBraveWalletPolkadotChainAssets)
      .FindDict(chain_id);
}

base::DictValue* EnsureChainDict(ScopedDictPrefUpdate& update,
                                 std::string_view chain_id) {
  update->Set(kVersionField, PolkadotChainAssetsPref::kVersion);
  return update->EnsureDict(std::string(chain_id));
}

std::string AssetIdKey(uint32_t asset_id) {
  return base::NumberToString(asset_id);
}

}  // namespace

PolkadotChainAssetsPref::PolkadotChainAssetsPref(PrefService& profile_prefs)
    : profile_prefs_(profile_prefs) {
  ClearPrefsOnVersionMismatch();
}

PolkadotChainAssetsPref::~PolkadotChainAssetsPref() = default;

void PolkadotChainAssetsPref::ClearPrefsOnVersionMismatch() {
  const auto& all_assets =
      profile_prefs_->GetDict(kBraveWalletPolkadotChainAssets);
  const auto version = all_assets.FindInt(kVersionField);
  if (!version || *version != PolkadotChainAssetsPref::kVersion) {
    profile_prefs_->ClearPref(kBraveWalletPolkadotChainAssets);
  }
}

std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>
PolkadotChainAssetsPref::GetAssetStorageHash(std::string_view chain_id) const {
  const auto* chain_assets = GetChainDict(*profile_prefs_, chain_id);
  if (!chain_assets) {
    return std::nullopt;
  }
  return HashFromPrefString(*chain_assets, kAssetStorageHash);
}

bool PolkadotChainAssetsPref::SetAssetStorageHash(
    std::string_view chain_id,
    const std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>& hash) {
  ScopedDictPrefUpdate update(profile_prefs_.get(),
                              kBraveWalletPolkadotChainAssets);
  EnsureChainDict(update, chain_id)
      ->Set(kAssetStorageHash, HashToPrefString(hash));
  return true;
}

std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>
PolkadotChainAssetsPref::GetMetadataStorageHash(
    std::string_view chain_id) const {
  const auto* chain_assets = GetChainDict(*profile_prefs_, chain_id);
  if (!chain_assets) {
    return std::nullopt;
  }
  return HashFromPrefString(*chain_assets, kMetadataStorageHash);
}

bool PolkadotChainAssetsPref::SetMetadataStorageHash(
    std::string_view chain_id,
    const std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>& hash) {
  ScopedDictPrefUpdate update(profile_prefs_.get(),
                              kBraveWalletPolkadotChainAssets);
  EnsureChainDict(update, chain_id)
      ->Set(kMetadataStorageHash, HashToPrefString(hash));
  return true;
}

std::vector<uint32_t> PolkadotChainAssetsPref::GetSupportedAssets(
    std::string_view chain_id) const {
  const auto* chain_assets = GetChainDict(*profile_prefs_, chain_id);
  if (!chain_assets) {
    return {};
  }
  const auto* supported_asset_ids = chain_assets->FindList(kSupportedAssetIds);
  if (!supported_asset_ids) {
    return {};
  }

  std::vector<uint32_t> result;
  result.reserve(supported_asset_ids->size());
  for (const auto& value : *supported_asset_ids) {
    const auto* asset_id_string = value.GetIfString();
    uint32_t asset_id = 0;
    if (!asset_id_string || !base::StringToUint(*asset_id_string, &asset_id)) {
      return {};
    }
    result.push_back(asset_id);
  }
  return result;
}

bool PolkadotChainAssetsPref::SetSupportedAssets(
    std::string_view chain_id,
    base::span<const uint32_t> asset_ids) {
  base::ListValue value;
  for (uint32_t asset_id : asset_ids) {
    value.Append(AssetIdKey(asset_id));
  }

  ScopedDictPrefUpdate update(profile_prefs_.get(),
                              kBraveWalletPolkadotChainAssets);
  EnsureChainDict(update, chain_id)->Set(kSupportedAssetIds, std::move(value));
  return true;
}

std::optional<AssetMetadata> PolkadotChainAssetsPref::GetAssetMetadata(
    std::string_view chain_id,
    uint32_t asset_id) const {
  const auto* chain_assets = GetChainDict(*profile_prefs_, chain_id);
  if (!chain_assets) {
    return std::nullopt;
  }
  const auto* all_metadata = chain_assets->FindDict(kAssetMetadata);
  if (!all_metadata) {
    return std::nullopt;
  }
  const auto* metadata = all_metadata->FindDict(AssetIdKey(asset_id));
  if (!metadata) {
    return std::nullopt;
  }

  std::array<uint8_t, 16> deposit = {};
  const auto* deposit_string = metadata->FindString(kDeposit);
  if (!deposit_string || !PrefixedHexStringToFixed(*deposit_string, deposit)) {
    return std::nullopt;
  }

  std::vector<uint8_t> name;
  if (!ReadBytes(*metadata, kName, &name)) {
    return std::nullopt;
  }

  std::vector<uint8_t> symbol;
  if (!ReadBytes(*metadata, kSymbol, &symbol)) {
    return std::nullopt;
  }

  uint8_t decimals = 0;
  const auto decimals_int = metadata->FindInt(kDecimals);
  if (!decimals_int ||
      !base::CheckedNumeric<uint8_t>(*decimals_int).AssignIfValid(&decimals)) {
    return std::nullopt;
  }

  const auto is_frozen = metadata->FindBool(kIsFrozen);
  if (!is_frozen.has_value()) {
    return std::nullopt;
  }

  return AssetMetadata::FromFields(deposit, std::move(name), std::move(symbol),
                                   decimals, *is_frozen);
}

bool PolkadotChainAssetsPref::SetAssetMetadata(std::string_view chain_id,
                                               uint32_t asset_id,
                                               const AssetMetadata& metadata) {
  base::DictValue value;
  value.Set(kDeposit, "0x" + base::HexEncodeLower(metadata.deposit()));
  value.Set(kName, "0x" + base::HexEncodeLower(metadata.name()));
  value.Set(kSymbol, "0x" + base::HexEncodeLower(metadata.symbol()));
  value.Set(kDecimals, static_cast<int>(metadata.decimals()));
  value.Set(kIsFrozen, metadata.is_frozen());

  ScopedDictPrefUpdate update(profile_prefs_.get(),
                              kBraveWalletPolkadotChainAssets);
  EnsureChainDict(update, chain_id)
      ->EnsureDict(kAssetMetadata)
      ->Set(AssetIdKey(asset_id), std::move(value));
  return true;
}

void PolkadotChainAssetsPref::ClearChainAssets(std::string_view chain_id) {
  ScopedDictPrefUpdate update(profile_prefs_.get(),
                              kBraveWalletPolkadotChainAssets);
  update->Set(kVersionField, PolkadotChainAssetsPref::kVersion);
  update->Remove(std::string(chain_id));
}

}  // namespace brave_wallet

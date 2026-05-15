/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_ASSETS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_ASSETS_H_

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/internal/polkadot_assets.rs.h"

namespace brave_wallet {

// Parsed asset identifiers from pallet-assets `Assets.Asset` storage keys
// returned by state_getKeysPaged.
class AssetsList {
 public:
  AssetsList() = delete;
  AssetsList(const AssetsList&);
  AssetsList(AssetsList&&) noexcept;
  ~AssetsList();

  AssetsList& operator=(const AssetsList&);
  AssetsList& operator=(AssetsList&&) noexcept;

  static std::optional<AssetsList> FromStorageKeys(
      base::span<const std::string> storage_keys);

  const std::vector<uint32_t>& identifiers() const { return identifiers_; }

 private:
  explicit AssetsList(std::vector<uint32_t> identifiers);

  std::vector<uint32_t> identifiers_;
};

// Parsed SCALE value stored under pallet-assets `Assets.Metadata(asset_id)`.
class AssetMetadata {
 public:
  AssetMetadata() = delete;
  AssetMetadata(const AssetMetadata&);
  AssetMetadata(AssetMetadata&&) noexcept;
  ~AssetMetadata();

  AssetMetadata& operator=(const AssetMetadata&);
  AssetMetadata& operator=(AssetMetadata&&) noexcept;
  bool operator==(const AssetMetadata& other) const;

  static std::optional<AssetMetadata> FromBytes(
      base::span<const uint8_t> metadata_bytes);
  static AssetMetadata FromFields(std::array<uint8_t, 16> deposit,
                                  std::vector<uint8_t> name,
                                  std::vector<uint8_t> symbol,
                                  uint8_t decimals,
                                  bool is_frozen);

  const CxxAssetMetadata& operator*() const;

  base::span<const uint8_t, 16> deposit() const;
  const std::vector<uint8_t>& name() const { return name_; }
  const std::vector<uint8_t>& symbol() const { return symbol_; }
  uint8_t decimals() const { return asset_metadata_.decimals; }
  bool is_frozen() const { return asset_metadata_.is_frozen; }

 private:
  AssetMetadata(CxxAssetMetadata asset_metadata,
                std::vector<uint8_t> name,
                std::vector<uint8_t> symbol);

  CxxAssetMetadata asset_metadata_;
  std::vector<uint8_t> name_;
  std::vector<uint8_t> symbol_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_ASSETS_H_

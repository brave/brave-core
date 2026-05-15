/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_CHAIN_ASSETS_PREF_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_CHAIN_ASSETS_PREF_H_

#include <array>
#include <optional>
#include <string_view>
#include <vector>

#include "base/containers/span.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_assets.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"

class PrefService;

namespace brave_wallet {

class PolkadotChainAssetsPref {
 public:
  static constexpr int kVersion = 1;

  explicit PolkadotChainAssetsPref(PrefService& profile_prefs);
  virtual ~PolkadotChainAssetsPref();

  virtual std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>
  GetAssetStorageHash(std::string_view chain_id) const;
  virtual bool SetAssetStorageHash(
      std::string_view chain_id,
      const std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>& hash);

  virtual std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>
  GetMetadataStorageHash(std::string_view chain_id) const;
  virtual bool SetMetadataStorageHash(
      std::string_view chain_id,
      const std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>& hash);

  virtual std::vector<uint32_t> GetSupportedAssets(
      std::string_view chain_id) const;
  virtual bool SetSupportedAssets(std::string_view chain_id,
                                  base::span<const uint32_t> asset_ids);

  virtual std::optional<AssetMetadata> GetAssetMetadata(
      std::string_view chain_id,
      uint32_t asset_id) const;
  virtual bool SetAssetMetadata(std::string_view chain_id,
                                uint32_t asset_id,
                                const AssetMetadata& metadata);

  virtual void ClearChainAssets(std::string_view chain_id);

 private:
  void ClearPrefsOnVersionMismatch();

  const raw_ref<PrefService> profile_prefs_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_CHAIN_ASSETS_PREF_H_

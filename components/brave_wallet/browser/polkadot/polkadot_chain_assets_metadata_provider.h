/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_CHAIN_ASSETS_METADATA_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_CHAIN_ASSETS_METADATA_PROVIDER_H_

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_assets.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_assets_pref.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"

namespace brave_wallet {

class PolkadotChainAssetsMetadataProvider {
 public:
  using ResolveMetadataCallback = base::OnceCallback<void(
      base::expected<std::optional<AssetMetadata>, std::string>)>;

  PolkadotChainAssetsMetadataProvider(
      PolkadotChainAssetsPref& chain_assets_pref,
      PolkadotSubstrateRpc& polkadot_substrate_rpc);
  ~PolkadotChainAssetsMetadataProvider();

  void ResolveMetadata(std::string_view chain_id,
                       uint32_t asset_id,
                       ResolveMetadataCallback callback);

 private:
  using StorageHash =
      std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>;

  void OnGetAssetStorageHash(
      std::string chain_id,
      uint32_t asset_id,
      ResolveMetadataCallback callback,
      base::expected<StorageHash, std::string> asset_storage_hash);
  void OnGetSupportedAssets(
      std::string chain_id,
      uint32_t asset_id,
      StorageHash asset_storage_hash,
      ResolveMetadataCallback callback,
      base::expected<std::vector<uint32_t>, std::string> supported_asset_ids);
  void CheckMetadataStorageHash(std::string chain_id,
                                uint32_t asset_id,
                                StorageHash asset_storage_hash,
                                std::vector<uint32_t> supported_asset_ids,
                                ResolveMetadataCallback callback);
  void OnGetMetadataStorageHash(
      std::string chain_id,
      uint32_t asset_id,
      StorageHash asset_storage_hash,
      std::vector<uint32_t> supported_asset_ids,
      ResolveMetadataCallback callback,
      base::expected<StorageHash, std::string> metadata_storage_hash);
  void OnGetAssetMetadata(std::string chain_id,
                          uint32_t asset_id,
                          ResolveMetadataCallback callback,
                          base::expected<std::optional<std::vector<uint8_t>>,
                                         std::string> metadata_bytes);

  const raw_ref<PolkadotChainAssetsPref> chain_assets_pref_;
  const raw_ref<PolkadotSubstrateRpc> polkadot_substrate_rpc_;
  base::WeakPtrFactory<PolkadotChainAssetsMetadataProvider> weak_ptr_factory_{
      this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_CHAIN_ASSETS_METADATA_PROVIDER_H_

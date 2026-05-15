/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_assets_metadata_provider.h"

#include <algorithm>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {
namespace {

bool ContainsAssetId(const std::vector<uint32_t>& supported_asset_ids,
                     uint32_t asset_id) {
  return std::ranges::contains(supported_asset_ids, asset_id);
}

}  // namespace

PolkadotChainAssetsMetadataProvider::PolkadotChainAssetsMetadataProvider(
    PolkadotChainAssetsPref& chain_assets_pref,
    PolkadotSubstrateRpc& polkadot_substrate_rpc)
    : chain_assets_pref_(chain_assets_pref),
      polkadot_substrate_rpc_(polkadot_substrate_rpc) {}

PolkadotChainAssetsMetadataProvider::~PolkadotChainAssetsMetadataProvider() =
    default;

void PolkadotChainAssetsMetadataProvider::ResolveMetadata(
    std::string_view chain_id,
    uint32_t asset_id,
    ResolveMetadataCallback callback) {
  CHECK(IsPolkadotNetwork(chain_id));

  polkadot_substrate_rpc_->GetStorageHash(
      chain_id, PolkadotSubstrateRpc::GetAssetsAssetStoragePrefix(),
      base::BindOnce(
          &PolkadotChainAssetsMetadataProvider::OnGetAssetStorageHash,
          weak_ptr_factory_.GetWeakPtr(), std::string(chain_id), asset_id,
          std::move(callback)));
}

void PolkadotChainAssetsMetadataProvider::OnGetAssetStorageHash(
    std::string chain_id,
    uint32_t asset_id,
    ResolveMetadataCallback callback,
    base::expected<StorageHash, std::string> asset_storage_hash) {
  if (!asset_storage_hash.has_value()) {
    std::move(callback).Run(base::unexpected(asset_storage_hash.error()));
    return;
  }

  auto saved_asset_storage_hash =
      chain_assets_pref_->GetAssetStorageHash(chain_id);
  auto supported_asset_ids = chain_assets_pref_->GetSupportedAssets(chain_id);
  if (saved_asset_storage_hash == *asset_storage_hash &&
      !supported_asset_ids.empty()) {
    CheckMetadataStorageHash(
        std::move(chain_id), asset_id, std::move(*asset_storage_hash),
        std::move(supported_asset_ids), std::move(callback));
    return;
  }

  chain_assets_pref_->ClearChainAssets(chain_id);
  chain_assets_pref_->SetAssetStorageHash(chain_id, *asset_storage_hash);

  polkadot_substrate_rpc_->GetSupportedAssets(
      chain_id,
      base::BindOnce(&PolkadotChainAssetsMetadataProvider::OnGetSupportedAssets,
                     weak_ptr_factory_.GetWeakPtr(), std::move(chain_id),
                     asset_id, std::move(*asset_storage_hash),
                     std::move(callback)));
}

void PolkadotChainAssetsMetadataProvider::OnGetSupportedAssets(
    std::string chain_id,
    uint32_t asset_id,
    StorageHash asset_storage_hash,
    ResolveMetadataCallback callback,
    base::expected<std::vector<uint32_t>, std::string> supported_asset_ids) {
  if (!supported_asset_ids.has_value()) {
    std::move(callback).Run(base::unexpected(supported_asset_ids.error()));
    return;
  }

  if (!chain_assets_pref_->SetSupportedAssets(chain_id, *supported_asset_ids)) {
    std::move(callback).Run(base::unexpected(WalletInternalErrorMessage()));
    return;
  }

  CheckMetadataStorageHash(
      std::move(chain_id), asset_id, std::move(asset_storage_hash),
      std::move(*supported_asset_ids), std::move(callback));
}

void PolkadotChainAssetsMetadataProvider::CheckMetadataStorageHash(
    std::string chain_id,
    uint32_t asset_id,
    StorageHash asset_storage_hash,
    std::vector<uint32_t> supported_asset_ids,
    ResolveMetadataCallback callback) {
  if (!ContainsAssetId(supported_asset_ids, asset_id)) {
    std::move(callback).Run(base::unexpected("Unsupported Polkadot asset id."));
    return;
  }

  polkadot_substrate_rpc_->GetStorageHash(
      chain_id, PolkadotSubstrateRpc::GetAssetsMetadataStoragePrefix(),
      base::BindOnce(
          &PolkadotChainAssetsMetadataProvider::OnGetMetadataStorageHash,
          weak_ptr_factory_.GetWeakPtr(), std::move(chain_id), asset_id,
          std::move(asset_storage_hash), std::move(supported_asset_ids),
          std::move(callback)));
}

void PolkadotChainAssetsMetadataProvider::OnGetMetadataStorageHash(
    std::string chain_id,
    uint32_t asset_id,
    StorageHash asset_storage_hash,
    std::vector<uint32_t> supported_asset_ids,
    ResolveMetadataCallback callback,
    base::expected<StorageHash, std::string> metadata_storage_hash) {
  if (!metadata_storage_hash.has_value()) {
    std::move(callback).Run(base::unexpected(metadata_storage_hash.error()));
    return;
  }

  auto saved_metadata_storage_hash =
      chain_assets_pref_->GetMetadataStorageHash(chain_id);
  if (saved_metadata_storage_hash != *metadata_storage_hash) {
    chain_assets_pref_->ClearChainAssets(chain_id);
    chain_assets_pref_->SetAssetStorageHash(chain_id, asset_storage_hash);
    chain_assets_pref_->SetSupportedAssets(chain_id, supported_asset_ids);
    chain_assets_pref_->SetMetadataStorageHash(chain_id,
                                               *metadata_storage_hash);
  }

  auto cached_metadata =
      chain_assets_pref_->GetAssetMetadata(chain_id, asset_id);
  if (cached_metadata) {
    std::move(callback).Run(base::ok(std::move(cached_metadata)));
    return;
  }

  polkadot_substrate_rpc_->GetAssetMetadata(
      chain_id, asset_id,
      base::BindOnce(&PolkadotChainAssetsMetadataProvider::OnGetAssetMetadata,
                     weak_ptr_factory_.GetWeakPtr(), std::move(chain_id),
                     asset_id, std::move(callback)));
}

void PolkadotChainAssetsMetadataProvider::OnGetAssetMetadata(
    std::string chain_id,
    uint32_t asset_id,
    ResolveMetadataCallback callback,
    base::expected<std::optional<std::vector<uint8_t>>, std::string>
        metadata_bytes) {
  if (!metadata_bytes.has_value()) {
    std::move(callback).Run(base::unexpected(metadata_bytes.error()));
    return;
  }

  if (!*metadata_bytes) {
    std::move(callback).Run(base::ok(std::nullopt));
    return;
  }

  auto metadata = AssetMetadata::FromBytes(**metadata_bytes);
  if (!metadata) {
    std::move(callback).Run(base::unexpected(WalletParsingErrorMessage()));
    return;
  }

  if (!chain_assets_pref_->SetAssetMetadata(chain_id, asset_id, *metadata)) {
    std::move(callback).Run(base::unexpected(WalletInternalErrorMessage()));
    return;
  }

  std::move(callback).Run(base::ok(std::move(metadata)));
}

}  // namespace brave_wallet

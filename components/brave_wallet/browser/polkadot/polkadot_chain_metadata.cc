/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata.h"

#include "brave/components/brave_wallet/browser/internal/polkadot_chain_metadata.rs.h"

namespace brave_wallet {

PolkadotChainMetadata::PolkadotChainMetadata(const PolkadotChainMetadata& other)
    : chain_metadata_(other.chain_metadata_) {}

PolkadotChainMetadata::PolkadotChainMetadata(PolkadotChainMetadata&&) noexcept =
    default;

PolkadotChainMetadata::~PolkadotChainMetadata() = default;

PolkadotChainMetadata& PolkadotChainMetadata::operator=(
    const PolkadotChainMetadata&) = default;

PolkadotChainMetadata& PolkadotChainMetadata::operator=(
    PolkadotChainMetadata&&) noexcept = default;

bool PolkadotChainMetadata::operator==(
    const PolkadotChainMetadata& other) const {
  return chain_metadata_ == other.chain_metadata_;
}

std::optional<PolkadotChainMetadata> PolkadotChainMetadata::FromBytes(
    base::span<const uint8_t> metadata_bytes) {
  auto result = parse_chain_metadata_from_scale(
      ::rust::Slice<const uint8_t>(metadata_bytes));
  if (!result->is_ok()) {
    return std::nullopt;
  }

  auto parsed = result->unwrap();
  return FromFields(parsed->system_pallet_index(),
                    parsed->balances_pallet_index(),
                    parsed->transaction_payment_pallet_index(),
                    parsed->transfer_allow_death_call_index(),
                    parsed->transfer_keep_alive_call_index(),
                    parsed->transfer_all_call_index(), parsed->ss58_prefix(),
                    parsed->spec_version(), parsed->asset_tx_payment(),
                    parsed->has_assets_pallet(), parsed->assets_pallet_index(),
                    parsed->assets_asset_storage_index(),
                    parsed->assets_metadata_storage_index(),
                    parsed->assets_account_storage_index(),
                    parsed->assets_transfer_keep_alive_call_index());
}

// static
PolkadotChainMetadata PolkadotChainMetadata::FromFields(
    uint8_t system_pallet_index,
    uint8_t balances_pallet_index,
    uint8_t transaction_payment_pallet_index,
    uint8_t transfer_allow_death_call_index,
    uint8_t transfer_keep_alive_call_index,
    uint8_t transfer_all_call_index,
    uint16_t ss58_prefix,
    uint32_t spec_version,
    bool asset_tx_payment,
    bool has_assets_pallet,
    uint8_t assets_pallet_index,
    uint8_t assets_asset_storage_index,
    uint8_t assets_metadata_storage_index,
    uint8_t assets_account_storage_index,
    uint8_t assets_transfer_keep_alive_call_index) {
  CxxPolkadotChainMetadata metadata;
  metadata.system_pallet_index = system_pallet_index;
  metadata.balances_pallet_index = balances_pallet_index;
  metadata.transaction_payment_pallet_index = transaction_payment_pallet_index;
  metadata.transfer_allow_death_call_index = transfer_allow_death_call_index;
  metadata.transfer_keep_alive_call_index = transfer_keep_alive_call_index;
  metadata.transfer_all_call_index = transfer_all_call_index;
  metadata.assets_pallet_index = assets_pallet_index;
  metadata.assets_asset_storage_index = assets_asset_storage_index;
  metadata.assets_metadata_storage_index = assets_metadata_storage_index;
  metadata.assets_account_storage_index = assets_account_storage_index;
  metadata.assets_transfer_keep_alive_call_index =
      assets_transfer_keep_alive_call_index;
  metadata.has_assets_pallet = has_assets_pallet;
  metadata.ss58_prefix = ss58_prefix;
  metadata.spec_version = spec_version;
  metadata.asset_tx_payment = asset_tx_payment;
  return PolkadotChainMetadata(metadata);
}

// spec_version is unknown when constructing from chain name alone; callers
// must update it from state_getRuntimeVersion before use.
constexpr uint32_t kUnknownSpecVersion = 0;

// static
std::optional<PolkadotChainMetadata> PolkadotChainMetadata::FromChainName(
    std::string_view chain_name) {
  // https://github.com/paritytech/polkadot-sdk/blob/69f210b33fce91b23570f3bda64f8e3deff04843/polkadot/runtime/westend/src/lib.rs#L1853-L1854
  if (chain_name == "Westend") {
    return FromFields(/*system_pallet_index=*/0,
                      /*balances_pallet_index=*/4,
                      /*transaction_payment_pallet_index=*/0x1a,
                      /*transfer_allow_death_call_index=*/0,
                      /*transfer_keep_alive_call_index=*/3,
                      /*transfer_all_call_index=*/4,
                      /*ss58_prefix=*/42, kUnknownSpecVersion,
                      /*asset_tx_payment=*/false);
  }

  // https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/asset-hub-kusama-json.json#L969
  if (chain_name == "Westend Asset Hub") {
    return FromFields(/*system_pallet_index=*/0,
                      /*balances_pallet_index=*/10,
                      /*transaction_payment_pallet_index=*/0x0b,
                      /*transfer_allow_death_call_index=*/0,
                      /*transfer_keep_alive_call_index=*/3,
                      /*transfer_all_call_index=*/4,
                      /*ss58_prefix=*/42, kUnknownSpecVersion,
                      /*asset_tx_payment=*/true,
                      /*has_assets_pallet=*/true,
                      /*assets_pallet_index=*/50,
                      /*assets_asset_storage_index=*/0,
                      /*assets_metadata_storage_index=*/3,
                      /*assets_account_storage_index=*/1,
                      /*assets_transfer_keep_alive_call_index=*/9);
  }

  // https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/polkadot-json.json#L1096
  if (chain_name == "Polkadot") {
    return FromFields(/*system_pallet_index=*/0,
                      /*balances_pallet_index=*/5,
                      /*transaction_payment_pallet_index=*/0x20,
                      /*transfer_allow_death_call_index=*/0,
                      /*transfer_keep_alive_call_index=*/3,
                      /*transfer_all_call_index=*/4,
                      /*ss58_prefix=*/0, kUnknownSpecVersion,
                      /*asset_tx_payment=*/false);
  }

  // https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/asset-hub-polkadot-json.json#L969
  if (chain_name == "Polkadot Asset Hub") {
    return FromFields(/*system_pallet_index=*/0,
                      /*balances_pallet_index=*/10,
                      /*transaction_payment_pallet_index=*/0x0b,
                      /*transfer_allow_death_call_index=*/0,
                      /*transfer_keep_alive_call_index=*/3,
                      /*transfer_all_call_index=*/4,
                      /*ss58_prefix=*/0, kUnknownSpecVersion,
                      /*asset_tx_payment=*/true,
                      /*has_assets_pallet=*/true,
                      /*assets_pallet_index=*/50,
                      /*assets_asset_storage_index=*/0,
                      /*assets_metadata_storage_index=*/3,
                      /*assets_account_storage_index=*/1,
                      /*assets_transfer_keep_alive_call_index=*/9);
  }

  return std::nullopt;
}

const CxxPolkadotChainMetadata& PolkadotChainMetadata::operator*() const {
  return chain_metadata_;
}

uint8_t PolkadotChainMetadata::GetSystemPalletIndex() const {
  return chain_metadata_.system_pallet_index;
}

uint8_t PolkadotChainMetadata::GetBalancesPalletIndex() const {
  return chain_metadata_.balances_pallet_index;
}

uint8_t PolkadotChainMetadata::GetTransactionPaymentPalletIndex() const {
  return chain_metadata_.transaction_payment_pallet_index;
}

uint8_t PolkadotChainMetadata::GetTransferAllowDeathCallIndex() const {
  return chain_metadata_.transfer_allow_death_call_index;
}

uint8_t PolkadotChainMetadata::GetTransferKeepAliveCallIndex() const {
  return chain_metadata_.transfer_keep_alive_call_index;
}

uint8_t PolkadotChainMetadata::GetTransferAllCallIndex() const {
  return chain_metadata_.transfer_all_call_index;
}

bool PolkadotChainMetadata::HasAssetsPallet() const {
  return chain_metadata_.has_assets_pallet;
}

uint8_t PolkadotChainMetadata::GetAssetsPalletIndex() const {
  return chain_metadata_.assets_pallet_index;
}

uint8_t PolkadotChainMetadata::GetAssetsAssetStorageIndex() const {
  return chain_metadata_.assets_asset_storage_index;
}

uint8_t PolkadotChainMetadata::GetAssetsMetadataStorageIndex() const {
  return chain_metadata_.assets_metadata_storage_index;
}

uint8_t PolkadotChainMetadata::GetAssetsAccountStorageIndex() const {
  return chain_metadata_.assets_account_storage_index;
}

uint8_t PolkadotChainMetadata::GetAssetsTransferKeepAliveCallIndex() const {
  return chain_metadata_.assets_transfer_keep_alive_call_index;
}

uint16_t PolkadotChainMetadata::GetSs58Prefix() const {
  return chain_metadata_.ss58_prefix;
}

uint32_t PolkadotChainMetadata::GetSpecVersion() const {
  return chain_metadata_.spec_version;
}

bool PolkadotChainMetadata::UsesAssetTxPayment() const {
  return chain_metadata_.asset_tx_payment;
}

PolkadotChainMetadata::PolkadotChainMetadata(
    CxxPolkadotChainMetadata chain_metadata)
    : chain_metadata_(chain_metadata) {}

}  // namespace brave_wallet

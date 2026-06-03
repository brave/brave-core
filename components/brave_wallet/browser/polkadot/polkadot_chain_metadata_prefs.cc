/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata_prefs.h"

#include <string>

#include "base/numerics/checked_math.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {
namespace {

constexpr char kBalancesPalletIndex[] = "balances_pallet_index";
constexpr char kSystemPalletIndex[] = "system_pallet_index";
constexpr char kTransactionPaymentPalletIndex[] =
    "transaction_payment_pallet_index";
constexpr char kTransferAllowDeathCallIndex[] =
    "transfer_allow_death_call_index";
constexpr char kTransferKeepAliveCallIndex[] = "transfer_keep_alive_call_index";
constexpr char kTransferAllCallIndex[] = "transfer_all_call_index";
constexpr char kHasAssetsPallet[] = "has_assets_pallet";
constexpr char kAssetsPalletIndex[] = "assets_pallet_index";
constexpr char kAssetsTransferAllCallIndex[] = "assets_transfer_all_call_index";
constexpr char kAssetsTransferKeepAliveCallIndex[] =
    "assets_transfer_keep_alive_call_index";
constexpr char kAssetTxPayment[] = "asset_tx_payment";
constexpr char kSs58Prefix[] = "ss58_prefix";
constexpr char kSpecVersion[] = "spec_version";
constexpr char kVersionField[] = "version";

// Pref structure stored under kBraveWalletPolkadotChainMetadata:
// {
//   "version": int,                             // metadata schema version
//   "<chain_id>": {
//     "system_pallet_index": int,               // u8
//     "balances_pallet_index": int,             // u8
//     "transaction_payment_pallet_index": int,  // u8
//     "transfer_allow_death_call_index": int,   // u8
//     "transfer_keep_alive_call_index": int,    // u8
//     "transfer_all_call_index": int,           // u8
//     "has_assets_pallet": bool,                // bool
//     "assets_pallet_index": int,               // u8
//     "assets_transfer_all_call_index": int,    // u8
//     "assets_transfer_keep_alive_call_index": int,  // u8
//     "asset_tx_payment": bool,                 // bool
//     "ss58_prefix": int,                       // u16
//     "spec_version": int                       // u32
//   },
//   ...
// }

template <class T>
bool ReadUint(const base::DictValue& dict,
              const char* key,
              T* converted_value) {
  auto value = dict.FindInt(key);
  if (!value ||
      !base::CheckedNumeric<T>(*value).AssignIfValid(converted_value)) {
    return false;
  }
  return true;
}

bool ReadBool(const base::DictValue& dict, const char* key, bool* out_value) {
  auto value = dict.FindBool(key);
  if (!value.has_value()) {
    return false;
  }

  *out_value = *value;
  return true;
}

}  // namespace

PolkadotChainMetadataPrefs::PolkadotChainMetadataPrefs(
    PrefService& profile_prefs)
    : profile_prefs_(profile_prefs) {
  ClearMetadataPrefsOnVersionMismatch();
}

PolkadotChainMetadataPrefs::~PolkadotChainMetadataPrefs() = default;

void PolkadotChainMetadataPrefs::ClearMetadataPrefsOnVersionMismatch() {
  const auto& all_metadata =
      profile_prefs_->GetDict(brave_wallet::kBraveWalletPolkadotChainMetadata);
  int version = 0;
  if (!ReadUint(all_metadata, kVersionField, &version) ||
      version != PolkadotChainMetadataPrefs::kVersion) {
    profile_prefs_->ClearPref(brave_wallet::kBraveWalletPolkadotChainMetadata);
  }
}

std::optional<PolkadotChainMetadata>
PolkadotChainMetadataPrefs::GetChainMetadata(std::string_view chain_id) const {
  const auto& all_metadata =
      profile_prefs_->GetDict(brave_wallet::kBraveWalletPolkadotChainMetadata);
  const auto* chain_metadata = all_metadata.FindDict(chain_id);
  if (!chain_metadata) {
    return std::nullopt;
  }

  PolkadotChainMetadata metadata;
  if (!ReadUint(*chain_metadata, kBalancesPalletIndex,
                &metadata->balances_pallet_index)) {
    return std::nullopt;
  }

  if (!ReadUint(*chain_metadata, kSystemPalletIndex,
                &metadata->system_pallet_index)) {
    return std::nullopt;
  }

  if (!ReadUint(*chain_metadata, kTransactionPaymentPalletIndex,
                &metadata->transaction_payment_pallet_index)) {
    return std::nullopt;
  }

  if (!ReadUint(*chain_metadata, kTransferAllowDeathCallIndex,
                &metadata->transfer_allow_death_call_index)) {
    return std::nullopt;
  }

  if (!ReadUint(*chain_metadata, kTransferKeepAliveCallIndex,
                &metadata->transfer_keep_alive_call_index)) {
    return std::nullopt;
  }

  if (!ReadUint(*chain_metadata, kTransferAllCallIndex,
                &metadata->transfer_all_call_index)) {
    return std::nullopt;
  }

  if (!ReadBool(*chain_metadata, kHasAssetsPallet,
                &metadata->has_assets_pallet)) {
    return std::nullopt;
  }

  if (!ReadUint(*chain_metadata, kAssetsPalletIndex,
                &metadata->assets_pallet_index)) {
    return std::nullopt;
  }

  if (!ReadUint(*chain_metadata, kAssetsTransferAllCallIndex,
                &metadata->assets_transfer_all_call_index)) {
    return std::nullopt;
  }

  if (!ReadUint(*chain_metadata, kAssetsTransferKeepAliveCallIndex,
                &metadata->assets_transfer_keep_alive_call_index)) {
    return std::nullopt;
  }

  if (!ReadBool(*chain_metadata, kAssetTxPayment,
                &metadata->asset_tx_payment)) {
    return std::nullopt;
  }

  if (!ReadUint(*chain_metadata, kSs58Prefix, &metadata->ss58_prefix)) {
    return std::nullopt;
  }

  if (!ReadUint(*chain_metadata, kSpecVersion, &metadata->spec_version)) {
    return std::nullopt;
  }

  return metadata;
}

bool PolkadotChainMetadataPrefs::SetChainMetadata(
    std::string_view chain_id,
    const PolkadotChainMetadata& metadata) {
  base::DictValue value;
  int balances_pallet_index = 0;
  int system_pallet_index = 0;
  int transaction_payment_pallet_index = 0;
  int transfer_allow_death_call_index = 0;
  int transfer_keep_alive_call_index = 0;
  int transfer_all_call_index = 0;
  int assets_pallet_index = 0;
  int assets_transfer_all_call_index = 0;
  int assets_transfer_keep_alive_call_index = 0;
  int ss58_prefix = 0;
  int spec_version = 0;
  if (!base::CheckedNumeric<int>(metadata->system_pallet_index)
           .AssignIfValid(&system_pallet_index) ||
      !base::CheckedNumeric<int>(metadata->balances_pallet_index)
           .AssignIfValid(&balances_pallet_index) ||
      !base::CheckedNumeric<int>(metadata->transaction_payment_pallet_index)
           .AssignIfValid(&transaction_payment_pallet_index) ||
      !base::CheckedNumeric<int>(metadata->transfer_allow_death_call_index)
           .AssignIfValid(&transfer_allow_death_call_index) ||
      !base::CheckedNumeric<int>(metadata->transfer_keep_alive_call_index)
           .AssignIfValid(&transfer_keep_alive_call_index) ||
      !base::CheckedNumeric<int>(metadata->transfer_all_call_index)
           .AssignIfValid(&transfer_all_call_index) ||
      !base::CheckedNumeric<int>(metadata->assets_pallet_index)
           .AssignIfValid(&assets_pallet_index) ||
      !base::CheckedNumeric<int>(metadata->assets_transfer_all_call_index)
           .AssignIfValid(&assets_transfer_all_call_index) ||
      !base::CheckedNumeric<int>(
           metadata->assets_transfer_keep_alive_call_index)
           .AssignIfValid(&assets_transfer_keep_alive_call_index) ||
      !base::CheckedNumeric<int>(metadata->ss58_prefix)
           .AssignIfValid(&ss58_prefix) ||
      !base::CheckedNumeric<int>(metadata->spec_version)
           .AssignIfValid(&spec_version)) {
    return false;
  }

  value.Set(kSystemPalletIndex, system_pallet_index);
  value.Set(kBalancesPalletIndex, balances_pallet_index);
  value.Set(kTransactionPaymentPalletIndex, transaction_payment_pallet_index);
  value.Set(kTransferAllowDeathCallIndex, transfer_allow_death_call_index);
  value.Set(kTransferKeepAliveCallIndex, transfer_keep_alive_call_index);
  value.Set(kTransferAllCallIndex, transfer_all_call_index);
  value.Set(kHasAssetsPallet, metadata->has_assets_pallet);
  value.Set(kAssetsPalletIndex, assets_pallet_index);
  value.Set(kAssetsTransferAllCallIndex, assets_transfer_all_call_index);
  value.Set(kAssetsTransferKeepAliveCallIndex,
            assets_transfer_keep_alive_call_index);
  value.Set(kAssetTxPayment, metadata->asset_tx_payment);
  value.Set(kSs58Prefix, ss58_prefix);
  value.Set(kSpecVersion, spec_version);
  ScopedDictPrefUpdate update(profile_prefs_.get(),
                              brave_wallet::kBraveWalletPolkadotChainMetadata);
  update->Set(kVersionField, PolkadotChainMetadataPrefs::kVersion);
  update->Set(std::string(chain_id), std::move(value));
  return true;
}

}  // namespace brave_wallet

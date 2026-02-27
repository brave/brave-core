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
constexpr char kTransferAllowDeathCallIndex[] =
    "transfer_allow_death_call_index";
constexpr char kSs58Prefix[] = "ss58_prefix";
constexpr char kSpecVersion[] = "spec_version";

// Pref structure stored under kBraveWalletPolkadotChainMetadata:
// {
//   "<chain_id>": {
//     "balances_pallet_index": int,             // u8
//     "transfer_allow_death_call_index": int,   // u8
//     "ss58_prefix": int,                       // u16
//     "spec_version": int                       // u32
//   },
//   ...
// }

std::optional<uint8_t> ReadUint8(const base::DictValue& dict, const char* key) {
  auto value = dict.FindInt(key);
  uint8_t converted_value = 0;
  if (!value ||
      !base::CheckedNumeric<uint8_t>(*value).AssignIfValid(&converted_value)) {
    return std::nullopt;
  }
  return converted_value;
}

std::optional<uint16_t> ReadUint16(const base::DictValue& dict,
                                   const char* key) {
  auto value = dict.FindInt(key);
  uint16_t converted_value = 0;
  if (!value ||
      !base::CheckedNumeric<uint16_t>(*value).AssignIfValid(&converted_value)) {
    return std::nullopt;
  }
  return converted_value;
}

std::optional<uint32_t> ReadUint32(const base::DictValue& dict,
                                   const char* key) {
  auto value = dict.FindInt(key);
  uint32_t converted_value = 0;
  if (!value ||
      !base::CheckedNumeric<uint32_t>(*value).AssignIfValid(&converted_value)) {
    return std::nullopt;
  }
  return converted_value;
}

}  // namespace

PolkadotChainMetadataPrefs::PolkadotChainMetadataPrefs(
    PrefService& profile_prefs)
    : profile_prefs_(profile_prefs) {}

PolkadotChainMetadataPrefs::~PolkadotChainMetadataPrefs() = default;

std::optional<PolkadotChainMetadata>
PolkadotChainMetadataPrefs::GetChainMetadata(std::string_view chain_id) const {
  const auto& all_metadata =
      profile_prefs_->GetDict(brave_wallet::kBraveWalletPolkadotChainMetadata);
  const auto* chain_metadata = all_metadata.FindDict(chain_id);
  if (!chain_metadata) {
    return std::nullopt;
  }

  auto balances_pallet_index = ReadUint8(*chain_metadata, kBalancesPalletIndex);
  auto transfer_allow_death_call_index =
      ReadUint8(*chain_metadata, kTransferAllowDeathCallIndex);
  auto ss58_prefix = ReadUint16(*chain_metadata, kSs58Prefix);
  auto spec_version = ReadUint32(*chain_metadata, kSpecVersion);
  if (!balances_pallet_index || !transfer_allow_death_call_index ||
      !ss58_prefix || !spec_version) {
    return std::nullopt;
  }

  return PolkadotChainMetadata::FromFields(
      *balances_pallet_index, *transfer_allow_death_call_index, *ss58_prefix,
      *spec_version);
}

bool PolkadotChainMetadataPrefs::SetChainMetadata(
    std::string_view chain_id,
    const PolkadotChainMetadata& metadata) {
  base::DictValue value;
  int balances_pallet_index = 0;
  int transfer_allow_death_call_index = 0;
  int ss58_prefix = 0;
  int spec_version = 0;
  if (!base::CheckedNumeric<int>(metadata.GetBalancesPalletIndex())
           .AssignIfValid(&balances_pallet_index) ||
      !base::CheckedNumeric<int>(metadata.GetTransferAllowDeathCallIndex())
           .AssignIfValid(&transfer_allow_death_call_index) ||
      !base::CheckedNumeric<int>(metadata.GetSs58Prefix())
           .AssignIfValid(&ss58_prefix) ||
      !base::CheckedNumeric<int>(metadata.GetSpecVersion())
           .AssignIfValid(&spec_version)) {
    return false;
  }

  value.Set(kBalancesPalletIndex, balances_pallet_index);
  value.Set(kTransferAllowDeathCallIndex, transfer_allow_death_call_index);
  value.Set(kSs58Prefix, ss58_prefix);
  value.Set(kSpecVersion, spec_version);

  ScopedDictPrefUpdate update(profile_prefs_.get(),
                              brave_wallet::kBraveWalletPolkadotChainMetadata);
  update->Set(std::string(chain_id), std::move(value));
  return true;
}

}  // namespace brave_wallet

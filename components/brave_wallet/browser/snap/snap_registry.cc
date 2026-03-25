/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_registry.h"

#include "base/no_destructor.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_service.h"

namespace brave_wallet {

namespace {

// Must match kInstalledSnapsPref in snap_installer.cc.
constexpr char kInstalledSnapsPref[] = "brave.wallet.installed_snaps";

// Reads a SnapRpcEndowment from a pref dict's "rpc" sub-dict.
SnapRpcEndowment RpcEndowmentFromPref(const base::Value::Dict& dict) {
  SnapRpcEndowment result;
  const base::Value::Dict* rpc = dict.FindDict("rpc");
  if (!rpc) {
    return result;
  }
  result.allow_dapps = rpc->FindBool("dapps").value_or(false);
  result.allow_snaps = rpc->FindBool("snaps").value_or(false);
  if (const base::Value::List* origins = rpc->FindList("allowedOrigins")) {
    for (const auto& item : *origins) {
      if (item.is_string()) {
        result.allowed_origins.push_back(item.GetString());
      }
    }
  }
  return result;
}

// Converts a pref entry for an installed snap into a SnapManifest.
// |snap_id| is the dict key; |dict| is the value dict.
SnapManifest ManifestFromPref(const std::string& snap_id,
                               const base::Value::Dict& dict) {
  SnapManifest manifest;
  manifest.snap_id = snap_id;
  manifest.resource_id = 0;  // Not bundled — loaded from file storage.

  if (const std::string* v = dict.FindString("version")) {
    manifest.version = *v;
  }
  if (const base::Value::List* list = dict.FindList("permissions")) {
    for (const auto& item : *list) {
      if (item.is_string()) {
        manifest.allowed_permissions.push_back(item.GetString());
      }
    }
  }
  manifest.endowment_rpc = RpcEndowmentFromPref(dict);
  return manifest;
}

}  // namespace

// ---------------------------------------------------------------------------
// SnapRpcEndowment
// ---------------------------------------------------------------------------

SnapRpcEndowment::SnapRpcEndowment() = default;
SnapRpcEndowment::SnapRpcEndowment(const SnapRpcEndowment&) = default;
SnapRpcEndowment& SnapRpcEndowment::operator=(const SnapRpcEndowment&) =
    default;
SnapRpcEndowment::SnapRpcEndowment(SnapRpcEndowment&&) = default;
SnapRpcEndowment& SnapRpcEndowment::operator=(SnapRpcEndowment&&) = default;
SnapRpcEndowment::~SnapRpcEndowment() = default;

// ---------------------------------------------------------------------------
// SnapManifest
// ---------------------------------------------------------------------------

SnapManifest::SnapManifest() = default;
SnapManifest::SnapManifest(const SnapManifest&) = default;
SnapManifest& SnapManifest::operator=(const SnapManifest&) = default;
SnapManifest::SnapManifest(SnapManifest&&) = default;
SnapManifest& SnapManifest::operator=(SnapManifest&&) = default;
SnapManifest::~SnapManifest() = default;

// ---------------------------------------------------------------------------
// SnapRegistry
// ---------------------------------------------------------------------------

// static
const std::vector<SnapManifest>& SnapRegistry::GetBuiltinSnaps() {
  static const base::NoDestructor<std::vector<SnapManifest>> kBuiltinSnaps([] {
    std::vector<SnapManifest> snaps;

    // Cosmos snap — derives Cosmos keys via snap_getBip44Entropy(coinType=118).
    // Bundle is embedded via IDR_BRAVE_WALLET_COSMOS_SNAP_JS.
    SnapManifest cosmos;
    cosmos.snap_id             = "npm:@cosmsnap/snap";
    cosmos.version             = "0.1.22";
    cosmos.allowed_permissions = {"snap_getBip44Entropy"};
    cosmos.resource_id         = IDR_BRAVE_WALLET_COSMOS_SNAP_JS;
    snaps.push_back(std::move(cosmos));

    // Filsnap — derives Filecoin keys via snap_getBip44Entropy(coinType=1 for
    // testnet, coinType=461 for mainnet). Uses snap_dialog for confirmations,
    // snap_manageState for persistent state, endowment:page-home for homepage.
    // Bundle is embedded via IDR_BRAVE_WALLET_FILECOIN_SNAP_JS.
    SnapManifest filecoin;
    filecoin.snap_id             = "npm:filsnap";
    filecoin.version             = "1.6.1";
    filecoin.allowed_permissions = {"snap_getBip44Entropy", "snap_dialog",
                                    "snap_manageState"};
    filecoin.resource_id         = IDR_BRAVE_WALLET_FILECOIN_SNAP_JS;
    snaps.push_back(std::move(filecoin));

    // PolkaGate snap — Polkadot ecosystem snap with homepage support.
    // Uses snap_getBip44Entropy (coinType=354 for DOT, coinType=434 for KSM),
    // snap_dialog for confirmations, snap_manageState, endowment:page-home.
    // Bundle is embedded via IDR_BRAVE_WALLET_POLKADOT_SNAP_JS.
    SnapManifest polkadot;
    polkadot.snap_id             = "npm:@polkagate/snap";
    polkadot.version             = "2.5.1";
    polkadot.allowed_permissions = {"snap_getBip44Entropy", "snap_dialog",
                                    "snap_manageState"};
    polkadot.resource_id         = IDR_BRAVE_WALLET_POLKADOT_SNAP_JS;
    snaps.push_back(std::move(polkadot));

    return snaps;
  }());
  return *kBuiltinSnaps;
}

SnapRegistry::SnapRegistry(PrefService* prefs) : prefs_(prefs) {
  DCHECK(prefs_);

  // Populate in-memory map from the persisted installed-snaps pref.
  const base::Value::Dict& all = prefs_->GetDict(kInstalledSnapsPref);
  for (const auto [snap_id, value] : all) {
    if (value.is_dict()) {
      installed_snaps_[snap_id] = ManifestFromPref(snap_id, value.GetDict());
    }
  }
}

SnapRegistry::~SnapRegistry() = default;

std::optional<SnapManifest> SnapRegistry::GetManifest(
    const std::string& snap_id) const {
  // Built-in snaps take priority.
  for (const auto& manifest : GetBuiltinSnaps()) {
    if (manifest.snap_id == snap_id) {
      return manifest;
    }
  }
  // Fall back to dynamically installed snaps.
  auto it = installed_snaps_.find(snap_id);
  if (it != installed_snaps_.end()) {
    return it->second;
  }
  return std::nullopt;
}

bool SnapRegistry::IsKnownSnap(const std::string& snap_id) const {
  return GetManifest(snap_id).has_value();
}

std::vector<SnapManifest> SnapRegistry::GetAllSnaps() const {
  std::vector<SnapManifest> result = GetBuiltinSnaps();
  for (const auto& [_, manifest] : installed_snaps_) {
    result.push_back(manifest);
  }
  return result;
}

void SnapRegistry::RegisterInstalledSnap(
    const std::string& snap_id,
    const std::string& version,
    const std::vector<std::string>& permissions,
    const SnapRpcEndowment& endowment_rpc) {
  SnapManifest manifest;
  manifest.snap_id = snap_id;
  manifest.version = version;
  manifest.allowed_permissions = permissions;
  manifest.endowment_rpc = endowment_rpc;
  manifest.resource_id = 0;
  installed_snaps_[snap_id] = std::move(manifest);
}

void SnapRegistry::UnregisterSnap(const std::string& snap_id) {
  installed_snaps_.erase(snap_id);
}

}  // namespace brave_wallet

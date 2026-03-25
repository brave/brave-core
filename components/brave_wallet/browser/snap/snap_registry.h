/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_REGISTRY_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_REGISTRY_H_

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"

class PrefService;

namespace brave_wallet {

// Parsed content of the snap manifest's `endowment:rpc` permission.
// Controls which callers may invoke the snap's onRpcRequest handler.
struct SnapRpcEndowment {
  SnapRpcEndowment();
  SnapRpcEndowment(const SnapRpcEndowment&);
  SnapRpcEndowment& operator=(const SnapRpcEndowment&);
  SnapRpcEndowment(SnapRpcEndowment&&);
  SnapRpcEndowment& operator=(SnapRpcEndowment&&);
  ~SnapRpcEndowment();

  bool allow_dapps = false;  // websites (non-snap origins) may call
  bool allow_snaps = false;  // other snaps may call (future)
  // Exact serialized origins allowed. Empty = allow all that pass dapps/snaps.
  std::vector<std::string> allowed_origins;
};

// Manifest describing a snap — either built-in (bundled at compile time) or
// dynamically installed from the npm registry at runtime.
struct SnapManifest {
  SnapManifest();
  SnapManifest(const SnapManifest&);
  SnapManifest& operator=(const SnapManifest&);
  SnapManifest(SnapManifest&&);
  SnapManifest& operator=(SnapManifest&&);
  ~SnapManifest();

  // Snap identifier, e.g. "npm:@metamask/bitcoin-devnet-snap"
  std::string snap_id;
  // Semver version string.
  std::string version;
  // Snap API methods this snap is allowed to call (e.g. "snap_getBip44Entropy")
  std::vector<std::string> allowed_permissions;
  // Parsed endowment:rpc config — who may call this snap's onRpcRequest.
  SnapRpcEndowment endowment_rpc;
  // GRD resource ID for the snap JS bundle.
  // 0 means the snap is not bundled and must be read from file storage.
  int resource_id = 0;
};

// SnapRegistry is the unified snap catalogue — built-in and installed snaps.
//
// Built-in snaps are defined statically in snap_registry.cc and are always
// available regardless of user action.
//
// Installed snaps are loaded from PrefService on construction and updated at
// runtime via RegisterInstalledSnap / UnregisterSnap as the user installs and
// removes snaps.
//
// Owned by SnapController.
class SnapRegistry {
 public:
  // Constructs the registry and populates the installed-snaps in-memory map
  // from the persisted PrefService data.
  explicit SnapRegistry(PrefService* prefs);
  ~SnapRegistry();

  SnapRegistry(const SnapRegistry&) = delete;
  SnapRegistry& operator=(const SnapRegistry&) = delete;

  // Returns the manifest for |snap_id| if it is in either the built-in
  // allowlist or the installed-snaps set, or std::nullopt otherwise.
  std::optional<SnapManifest> GetManifest(const std::string& snap_id) const;

  // Returns true if |snap_id| is built-in or installed.
  bool IsKnownSnap(const std::string& snap_id) const;

  // Returns all snaps — built-in followed by installed.
  std::vector<SnapManifest> GetAllSnaps() const;

  // Adds (or replaces) an installed snap in the in-memory map.
  // Called by SnapController after a successful InstallSnap pipeline.
  void RegisterInstalledSnap(const std::string& snap_id,
                              const std::string& version,
                              const std::vector<std::string>& permissions,
                              const SnapRpcEndowment& endowment_rpc);

  // Removes an installed snap from the in-memory map.
  // Called by SnapController after UninstallSnap.
  void UnregisterSnap(const std::string& snap_id);

 private:
  static const std::vector<SnapManifest>& GetBuiltinSnaps();

  // In-memory cache of installed snaps, keyed by snap_id.
  // Populated from PrefService on construction; kept in sync at runtime.
  std::map<std::string, SnapManifest> installed_snaps_;

  raw_ptr<PrefService> prefs_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_REGISTRY_H_

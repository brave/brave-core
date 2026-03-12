/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_REGISTRY_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_REGISTRY_H_

#include <optional>
#include <string>
#include <vector>

namespace brave_wallet {

// Manifest describing a built-in (bundled) snap.
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
  // GRD resource ID for the snap JS bundle. 0 means the snap is not bundled
  // and must be fetched/installed separately.
  int resource_id = 0;
};

// SnapRegistry maintains the set of snaps that Brave ships or allows.
//
// Initially this is a static allowlist. Future work may add a dynamic
// installed-snap store backed by PrefService.
class SnapRegistry {
 public:
  SnapRegistry() = delete;

  // Returns the manifest for |snap_id| if it is in the built-in allowlist,
  // or std::nullopt if the snap is unknown / not permitted.
  static std::optional<SnapManifest> GetManifest(const std::string& snap_id);

  // Returns true if |snap_id| is in the built-in allowlist.
  static bool IsKnownSnap(const std::string& snap_id);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_REGISTRY_H_

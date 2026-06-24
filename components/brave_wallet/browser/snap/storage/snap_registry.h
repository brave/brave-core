/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_STORAGE_SNAP_REGISTRY_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_STORAGE_SNAP_REGISTRY_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

class PrefRegistrySimple;
class PrefService;

namespace brave_wallet {

// SnapRegistry is the snap catalogue for runtime-installed snaps.
// Snaps are loaded from PrefService on construction and updated at runtime
// via RegisterInstalledSnap / UnregisterSnap as the user installs and removes
// snaps. All snap metadata is stored and returned as mojom::SnapInstallData.
class SnapRegistry {
 public:
  // Registers kInstalledSnapsPref. Call once during profile pref registration.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // Constructs the registry and populates the installed-snaps in-memory map
  // from the persisted PrefService data.
  explicit SnapRegistry(PrefService& prefs);
  ~SnapRegistry();

  SnapRegistry(const SnapRegistry&) = delete;
  SnapRegistry& operator=(const SnapRegistry&) = delete;

  // Returns a clone of the snap's install data, or nullptr if not installed.
  mojom::SnapInstallDataPtr GetSnap(const std::string& snap_id) const;

  // Returns true if |snap_id| is installed.
  bool IsKnownSnap(const std::string& snap_id) const;

  // Returns clones of all installed snaps.
  std::vector<mojom::SnapInstallDataPtr> GetAllSnaps() const;

  // Writes snap metadata to PrefService and updates the in-memory map.
  // Called by SnapDataProvider after a successful install pipeline.
  void RegisterInstalledSnap(const mojom::SnapInstallData& install_data);

  // Removes snap metadata from PrefService and from the in-memory map.
  // Called by SnapDataProvider during uninstall.
  void UnregisterSnap(const std::string& snap_id);

 private:
  // In-memory cache keyed by snap_id; populated from PrefService on
  // construction and kept in sync by Register/UnregisterInstalledSnap.
  std::map<std::string, mojom::SnapInstallDataPtr> installed_snaps_;

  raw_ref<PrefService> prefs_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_STORAGE_SNAP_REGISTRY_H_

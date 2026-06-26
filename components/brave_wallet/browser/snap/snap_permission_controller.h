/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_PERMISSION_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_PERMISSION_CONTROLLER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ref.h"

class PrefRegistrySimple;
class PrefService;

namespace url {
class Origin;
}

namespace brave_wallet {

class SnapDataProvider;

// SnapPermissionController manages per-origin snap connection grants.
//
// Connection grants are persisted in PrefService as:
//   "brave.wallet.connected_snaps": {
//     "https://app.example.com": ["npm:@metamask/some-snap", ...]
//   }
//
// IsOriginAllowedByManifest reads the snap's endowment:rpc manifest config
// from SnapDataProvider to determine whether a given origin may call a snap's
// onRpcRequest handler at all.
class SnapPermissionController {
 public:
  SnapPermissionController(PrefService& prefs, SnapDataProvider& data_provider);
  ~SnapPermissionController();

  SnapPermissionController(const SnapPermissionController&) = delete;
  SnapPermissionController& operator=(const SnapPermissionController&) = delete;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // Returns true if |origin| has an active connection grant to |snap_id|.
  bool IsSnapConnected(const url::Origin& origin,
                       const std::string& snap_id) const;

  // Persists a connection grant for (|origin|, |snap_id|). No-op if already
  // connected.
  void GrantSnapConnection(const url::Origin& origin,
                           const std::string& snap_id);

  // Removes the connection grant for (|origin|, |snap_id|).
  void RevokeSnapConnection(const url::Origin& origin,
                            const std::string& snap_id);

  // Returns all snap_ids connected to |origin|.
  std::vector<std::string> GetConnectedSnaps(const url::Origin& origin) const;

  // Returns the serialized origins currently connected to |snap_id| (the
  // reverse of GetConnectedSnaps).
  std::vector<std::string> GetOriginsConnectedToSnap(
      const std::string& snap_id) const;

  // Returns true if |origin| is allowed to invoke |snap_id| according to the
  // snap's endowment:rpc manifest config. Does NOT check connection grants.
  void IsOriginAllowedByManifest(const url::Origin& origin,
                                 const std::string& snap_id,
                                 base::OnceCallback<void(bool)> callback) const;

  // Checks whether |snap_id| has declared the permission required to call
  // |method|. Returns std::nullopt if the call is allowed. Returns an error
  // string if the snap is unknown or has not declared the required permission.
  // Handles the snap_confirm → snap_dialog alias internally.
  void CheckSnapMethodPermission(
      const std::string& snap_id,
      const std::string& method,
      base::OnceCallback<void(std::optional<std::string>)> callback) const;

  // Removes all connection grants for |snap_id| across every origin. Called
  // when a snap is uninstalled.
  void PurgeConnectionGrantsForSnap(const std::string& snap_id);

 private:
  raw_ref<PrefService> prefs_;
  raw_ref<SnapDataProvider> data_provider_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_PERMISSION_CONTROLLER_H_

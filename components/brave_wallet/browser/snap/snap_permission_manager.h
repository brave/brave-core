/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_PERMISSION_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_PERMISSION_MANAGER_H_

#include <string>

#include "base/memory/raw_ptr.h"

class PrefRegistrySimple;
class PrefService;

namespace url {
class Origin;
}  // namespace url

namespace brave_wallet {

// SnapPermissionManager stores per-origin snap permissions in PrefService.
//
// Permissions are keyed by origin serialisation → snap_id. Example pref layout:
//   "brave.wallet.snap_permissions": {
//     "https://app.uniswap.org": {
//       "npm:@metamask/eth-phishing-detect": true
//     }
//   }
class SnapPermissionManager {
 public:
  explicit SnapPermissionManager(PrefService* prefs);
  ~SnapPermissionManager();

  SnapPermissionManager(const SnapPermissionManager&) = delete;
  SnapPermissionManager& operator=(const SnapPermissionManager&) = delete;

  // Registers the preference key used by this class.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // Returns true if |origin| has been granted access to |snap_id|.
  bool HasPermission(const url::Origin& origin,
                     const std::string& snap_id) const;

  // Grants |origin| access to |snap_id|. Persisted immediately.
  void GrantPermission(const url::Origin& origin, const std::string& snap_id);

  // Revokes |origin|'s access to |snap_id|. Persisted immediately.
  void RevokePermission(const url::Origin& origin, const std::string& snap_id);

  // Revokes all snap permissions for |origin|.
  void RevokeAllPermissions(const url::Origin& origin);

 private:
  raw_ptr<PrefService> prefs_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_PERMISSION_MANAGER_H_

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_MANIFEST_HELPERS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_MANIFEST_HELPERS_H_

#include <string>
#include <string_view>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "url/origin.h"

namespace brave_wallet {

// Returns true when |name| is a permission Brave accepts in snap.manifest.json.
bool IsAllowedSnapPermission(std::string_view name);

// Canonical permission names for each set optional field on |manifest|.
std::vector<std::string> GetSnapPermissionNames(
    const mojom::SnapManifest& manifest);

// Returns true when |manifest| declares |name| (handles snap_confirm alias).
bool SnapManifestHasPermission(const mojom::SnapManifest& manifest,
                               std::string_view name);

// True when |manifest| grants |origin| RPC access via endowment:rpc.
bool SnapManifestAllowsOrigin(const mojom::SnapManifest& manifest,
                              const url::Origin& origin);

// Serializes |manifest| into a pref sub-dict.
base::DictValue SnapManifestToValue(const mojom::SnapManifest& manifest);

// Deserializes a pref sub-dict into mojom::SnapManifest.
mojom::SnapManifestPtr SnapManifestFromValue(const base::DictValue& dict);

// Parses a single initialPermissions entry into the matching |manifest| field.
// |permission_name| must already be allowlisted.
void ApplySnapPermissionFromValue(const std::string& permission_name,
                                  const base::Value& value,
                                  mojom::SnapManifest& manifest);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SNAP_MANIFEST_HELPERS_H_

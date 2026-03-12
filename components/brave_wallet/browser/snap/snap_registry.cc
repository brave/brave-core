/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_registry.h"

#include "base/containers/fixed_flat_map.h"
#include "base/no_destructor.h"

namespace brave_wallet {

SnapManifest::SnapManifest() = default;
SnapManifest::SnapManifest(const SnapManifest&) = default;
SnapManifest& SnapManifest::operator=(const SnapManifest&) = default;
SnapManifest::SnapManifest(SnapManifest&&) = default;
SnapManifest& SnapManifest::operator=(SnapManifest&&) = default;
SnapManifest::~SnapManifest() = default;

namespace {

// Built-in snap allowlist. Extend this list as additional snaps are bundled.
// Key: snap_id string.  Value: SnapManifest.
const auto& GetBuiltinSnaps() {
  // TODO(snap): Populate with real bundled snaps and their GRD resource IDs.
  static const base::NoDestructor<std::vector<SnapManifest>> kBuiltinSnaps({});
  return *kBuiltinSnaps;
}

}  // namespace

// static
std::optional<SnapManifest> SnapRegistry::GetManifest(
    const std::string& snap_id) {
  for (const auto& manifest : GetBuiltinSnaps()) {
    if (manifest.snap_id == snap_id) {
      return manifest;
    }
  }
  return std::nullopt;
}

// static
bool SnapRegistry::IsKnownSnap(const std::string& snap_id) {
  return GetManifest(snap_id).has_value();
}

}  // namespace brave_wallet

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/extensions/manifest_v2/brave_hosted_extensions.h"

namespace extensions_mv2 {

namespace {
consteval bool CheckExtensionMaps() {
  for (const auto& [bh_key, bh_value] : kBraveHosted) {
    if (bh_value.empty()) {
      // skip Brave-hosted extension which doesn't have the cws counterpart.
      continue;
    }

    bool ok = false;
    for (const auto& [cws_key, cws_value] : kCwsHosted) {
      if (bh_value == cws_key && bh_key == cws_value) {
        ok = true;
        break;
      }
    }
    if (!ok) {
      return false;
    }
  }
  return true;
}

static_assert(CheckExtensionMaps(),
              "kBraveHostedToCws & kCwsToBraveHosted aren't consistent");
}  // namespace

bool IsKnownMV2Extension(const extensions::ExtensionId& id) {
  return kBraveHosted.contains(id);
}

bool IsKnownCwsMV2Extension(const extensions::ExtensionId& id) {
  return kCwsHosted.contains(id);
}

std::optional<extensions::ExtensionId> GetBraveHostedExtensionId(
    const extensions::ExtensionId& cws_extension_id) {
  if (auto fnd = kCwsHosted.find(cws_extension_id); fnd != kCwsHosted.end()) {
    return extensions::ExtensionId(fnd->second);
  }
  return std::nullopt;
}

std::optional<extensions::ExtensionId> GetCwsExtensionId(
    const extensions::ExtensionId& brave_hosted_extension_id) {
  if (auto fnd = kBraveHosted.find(brave_hosted_extension_id);
      fnd != kBraveHosted.end()) {
    return extensions::ExtensionId(fnd->second);
  }
  return std::nullopt;
}

}  // namespace extensions_mv2

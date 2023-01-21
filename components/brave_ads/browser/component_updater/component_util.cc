/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/component_updater/component_util.h"

#include "brave/components/brave_ads/browser/component_updater/components.h"

namespace brave_ads {

absl::optional<ComponentInfo> GetComponentInfo(const std::string& id) {
  const auto iter = kComponents.find(id);
  if (iter == kComponents.end()) {
    return absl::nullopt;
  }

  return iter->second;
}

}  // namespace brave_ads

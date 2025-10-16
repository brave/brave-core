/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/virtual_pref_provider_util.h"

#include <cstddef>
#include <vector>

#include "base/version.h"
#include "base/version_info/version_info.h"

namespace brave_ads {

namespace {

int GetVersionComponent(size_t index) {
  const base::Version& version = version_info::GetVersion();
  const std::vector<uint32_t>& version_components = version.components();
  return index < version_components.size()
             ? static_cast<int>(version_components[index])
             : 0;
}

}  // namespace

int GetMajorVersion() {
  return GetVersionComponent(0);
}

int GetMinorVersion() {
  return GetVersionComponent(1);
}

int GetBuildVersion() {
  return GetVersionComponent(2);
}

int GetPatchVersion() {
  return GetVersionComponent(3);
}

}  // namespace brave_ads

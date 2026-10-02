/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_COMPONENT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_COMPONENT_INFO_H_

#include <array>
#include <cstdint>
#include <string_view>

#include "crypto/sha2.h"

namespace brave_ads {

struct ComponentInfo {
  std::string_view id;
  std::array<uint8_t, crypto::kSHA256Length> public_key_sha256;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_COMPONENT_INFO_H_

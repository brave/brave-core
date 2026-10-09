/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_COMPONENT_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_COMPONENT_UTIL_H_

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

#include "crypto/sha2.h"

namespace brave_ads {

// Returns the public key SHA256 hash of the component for the given resource
// id. The component's crx ID can be derived from it via
// crx_file::id_util::GenerateIdFromHash(). If no component is available for
// the given resource id, returns `std::nullopt`.
std::optional<std::array<uint8_t, crypto::kSHA256Length>>
GetComponentPublicKeySHA256(std::string_view resource_id);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_COMPONENT_UPDATER_COMPONENT_UTIL_H_

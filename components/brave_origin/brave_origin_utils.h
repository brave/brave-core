/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_UTILS_H_

#include <optional>
#include <string_view>

namespace brave_origin {

// Returns whether BraveOrigin is enabled for the current user.
// This checks the feature flag and will be updated to check actual
// purchase state from SKU service in the future.
bool IsBraveOriginEnabled();

// Gets the correct brave_origin_pref_key
std::string GetBraveOriginPrefKey(std::string_view policy_key,
                                  std::optional<std::string_view> profile_id);

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_UTILS_H_

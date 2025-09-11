/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_UTILS_H_

#include <string>

namespace brave_origin {

// Returns whether BraveOrigin is enabled for the current user.
// This checks the feature flag and will be updated to check actual
// purchase state from SKU service in the future.
bool IsBraveOriginEnabled();

// Forward declaration
struct BraveOriginPolicyInfo;

// Gets the correct brave_origin_pref_key for a preference based on its scope
// and the provided profile_id. For global prefs, returns pref_name directly.
// For profile prefs, returns "profile_id.pref_name".
std::string GetBraveOriginPrefKey(const BraveOriginPolicyInfo& pref_info,
                                  const std::string& profile_id);

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_BRAVE_ORIGIN_UTILS_H_

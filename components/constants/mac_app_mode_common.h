/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONSTANTS_MAC_APP_MODE_COMMON_H_
#define BRAVE_COMPONENTS_CONSTANTS_MAC_APP_MODE_COMMON_H_

namespace app_mode {

// Prefix applied to each launch URL passed to a directly-spawned test shim as a
// positional command-line argument. Such a shim does not receive URLs through
// -application:openURLs:, so it recovers them from its command line; the prefix
// lets it identify exactly which arguments are launch URLs instead of guessing
// from whether an argument parses as a URL. See web_app::LaunchShimForTesting
// and AppShimController.
inline constexpr char kTestLaunchUrlPrefix[] = "test-launch-url=";

}  // namespace app_mode

#endif  // BRAVE_COMPONENTS_CONSTANTS_MAC_APP_MODE_COMMON_H_

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_COMMON_UTIL_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_COMMON_UTIL_H_

class PrefService;

namespace web_discovery {

// Returns true if web discovery is enabled by user preference and not
// disabled by policy.
bool IsWebDiscoveryEnabled(PrefService& profile_prefs);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_COMMON_UTIL_H_

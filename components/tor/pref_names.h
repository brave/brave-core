/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_PREF_NAMES_H_
#define BRAVE_COMPONENTS_TOR_PREF_NAMES_H_

namespace tor::prefs {

inline constexpr char kTorDisabled[] = "tor.tor_disabled";
inline constexpr char kBridgesConfig[] = "tor.bridges";
inline constexpr char kBuiltinBridgesRequestTime[] =
    "tor.builtin_bridges_request_time";

// Automatically open onion available site or .onion domain in Tor window
inline constexpr char kAutoOnionRedirect[] = "tor.auto_onion_location";

// Restrict requests for .onion URLs to Tor windows
inline constexpr char kOnionOnlyInTorWindows[] =
    "tor.onion_only_in_tor_windows";

}  // namespace tor::prefs

#endif  // BRAVE_COMPONENTS_TOR_PREF_NAMES_H_

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_COMMON_PREF_NAMES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_COMMON_PREF_NAMES_H_

#include "build/build_config.h"

namespace prefs {
// Used by BraveVpnDnsObserverService to set cloudflare server url when
// BraveVPN is connected, otherwise this pref is empty. Final decision
// about whether or not to override actual DNS state will be made in
// stub_resolver_config_reader.
inline constexpr char kBraveVpnDnsConfig[] = "brave.brave_vpn.dns_config";
}  // namespace prefs

#include "src/chrome/common/pref_names.h"  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_CHROME_COMMON_PREF_NAMES_H_

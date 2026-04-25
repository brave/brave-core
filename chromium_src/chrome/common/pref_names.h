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

#if defined(TOOLKIT_VIEWS)
// Whether to show the share menu on the location bar.
inline constexpr char kPinShareMenuButton[] = "brave.pin_share_menu_button";

// A boolean specifying whether the PWA install button should be shown in the
// address bar.
inline constexpr char kPinPwaInstallButton[] = "browser.pin_pwa_install_button";
#endif  // defined(TOOLKIT_VIEWS)
}  // namespace prefs

#include <chrome/common/pref_names.h>  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_CHROME_COMMON_PREF_NAMES_H_

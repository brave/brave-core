/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_RESOURCES_RESOURCE_LOADER_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_RESOURCES_RESOURCE_LOADER_H_

#include <string>

namespace base {
class FilePath;
}  // namespace base

namespace brave_vpn {

// Loads the service's strings for the locale `SelectLocale()` picks.
void LoadLocaleResources();

// The directory holding the service's locale paks: `Locales` next to
// `assets_path`, or else in it.
base::FilePath FindLocalesDir(const base::FilePath& assets_path);

// Points `ui::DIR_LOCALES` at the paks under `assets_path`, and returns the
// OS's preferred UI language that has one, or "en-US", as the browser picks
// its locale. It becomes the default ICU locale too.
std::string SelectLocale(const base::FilePath& assets_path);

}  //  namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_RESOURCES_RESOURCE_LOADER_H_

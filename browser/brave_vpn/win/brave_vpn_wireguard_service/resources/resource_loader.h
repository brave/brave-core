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

void LoadLocaleResources();

base::FilePath FindPakFilePath(const base::FilePath& assets_path,
                               const std::string& locale);

}  //  namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_RESOURCES_RESOURCE_LOADER_H_

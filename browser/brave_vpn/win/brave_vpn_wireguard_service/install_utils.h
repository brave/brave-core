/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INSTALL_UTILS_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INSTALL_UTILS_H_

#include <string>

#include "base/files/file_path.h"

namespace base {
class FilePath;
}  // namespace base

namespace brave_vpn {

bool ConfigureBraveWireguardService(const std::wstring& service_name);
bool InstallBraveWireguardService(const base::FilePath& root_dir);
bool UninstallBraveWireguardService();
bool UninstallStatusTrayIcon();
bool InstallBraveVPNHelperService(const base::FilePath& root_dir);

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INSTALL_UTILS_H_

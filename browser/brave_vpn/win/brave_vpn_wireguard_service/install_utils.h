/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INSTALL_UTILS_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INSTALL_UTILS_H_

#include <string>

namespace base {
class CommandLine;
class FilePath;
}  // namespace base

class WorkItem;

namespace brave_vpn {

// Caller should take care of deleting the returned work item.
WorkItem* CreateWorkItemForWireguardServiceInstall(
    const base::CommandLine& service_cmd);
WorkItem* CreateWorkItemForVpnHelperServiceInstall(
    const base::CommandLine& service_cmd);

base::FilePath GetBraveVpnHelperServicePath();
bool ConfigureBraveWireguardService(const std::wstring& service_name);
bool InstallBraveWireguardService();
bool UninstallBraveWireguardService();
bool UninstallStatusTrayIcon();
bool InstallBraveVPNHelperService();

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_INSTALL_UTILS_H_

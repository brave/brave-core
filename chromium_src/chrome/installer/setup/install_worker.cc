/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <shlobj.h>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/version.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "build/buildflag.h"
#include "chrome/install_static/install_util.h"
#include "chrome/installer/setup/setup_util.h"
#include "chrome/installer/util/callback_work_item.h"
#include "chrome/installer/util/install_service_work_item.h"
#include "chrome/installer/util/work_item_list.h"

#if defined(OFFICIAL_BUILD)
// clang-format off
// NOLINTBEGIN(sort-order)
#include "chrome/install_static/buildflags.h"
#include "chrome/install_static/install_constants.h"
#include "chrome/install_static/brave_stash_google_update_integration.h"
#define BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION() (1)
// NOLINTEND(sort-order)
// clang-format on
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/browser/connection/ikev2/win/brave_vpn_helper/brave_vpn_helper_constants.h"
#include "brave/components/brave_vpn/browser/connection/ikev2/win/brave_vpn_helper/brave_vpn_helper_state.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_constants.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_details.h"

namespace {

base::FilePath GetBraveVPNHelperPath(const base::FilePath& target_path,
                                     const base::Version& version) {
  return target_path.AppendASCII(version.GetString())
      .Append(brave_vpn::kBraveVPNHelperExecutable);
}

// delete `BraveVpnWireguardService` from services and remove the tray icon.
bool UninstallBraveVPNWireguardService(const base::FilePath& exe_path,
                                       const CallbackWorkItem&) {
  if (!base::PathExists(exe_path)) {
    return false;
  }
  base::CommandLine cmd(exe_path);
  cmd.AppendSwitch(brave_vpn::kBraveVpnWireguardServiceUninstallSwitchName);
  base::LaunchOptions options = base::LaunchOptions();
  options.wait = true;
  return base::LaunchProcess(cmd, options).IsValid();
}

// Brave 1.50.114+ would register `BraveVpnService` for system level installs.
//
// This change removes the service if it exists. We'll be updating the browser
// to install the services post-purchase (if possible) when the user has
// purchased Brave VPN and has credentials or when VPN is used.
//
// See https://github.com/brave/brave-browser/issues/33726 for more info
void AddUninstallVpnServiceWorkItems(const base::FilePath& vpn_service_path,
                                     WorkItemList* list) {
  DCHECK(::IsUserAnAdmin());
  // delete `BraveVpnService` from services
  if (!installer::InstallServiceWorkItem::DeleteService(
          brave_vpn::GetBraveVpnHelperServiceName(),
          brave_vpn::kBraveVpnHelperRegistryStoragePath, {}, {})) {
    LOG(WARNING) << "Failed to delete "
                 << brave_vpn::GetBraveVpnHelperServiceName();
  }
}

// Brave 1.57.47+ would register `BraveVpnWireguardService` for system level
// installs.
//
// This change removes the service if it exists. We'll be updating the browser
// to install the services post-purchase (if possible) when the user has
// purchased Brave VPN and has credentials or when VPN is used.
//
// See https://github.com/brave/brave-browser/issues/33726 for more info
void AddUninstallWireguardServiceWorkItems(
    const base::FilePath& wireguard_service_path,
    WorkItemList* list) {
  DCHECK(::IsUserAnAdmin());
  list->AddCallbackWorkItem(base::BindOnce(&UninstallBraveVPNWireguardService,
                                           wireguard_service_path),
                            base::NullCallback());
}

}  // namespace

#define GetElevationServicePath GetElevationServicePath(                   \
  target_path, new_version), install_list);                                \
  AddUninstallWireguardServiceWorkItems(                                   \
      brave_vpn::GetBraveVPNWireguardServiceInstallationPath(target_path,  \
                                                             new_version), \
      install_list);                                                       \
  AddUninstallVpnServiceWorkItems(GetBraveVPNHelperPath

#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

#include "src/chrome/installer/setup/install_worker.cc"
#if BUILDFLAG(ENABLE_BRAVE_VPN)
#undef GetElevationServicePath
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)
#if defined(OFFICIAL_BUILD)
#include "chrome/install_static/brave_restore_google_update_integration.h"
#endif

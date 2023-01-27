/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <shlobj.h>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
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
#include "chrome/install_static/buildflags.h"
#include "chrome/install_static/install_constants.h"
#undef BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION
#define BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION() (1)
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/brave_vpn_helper_constants.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_helper/brave_vpn_helper_state.h"

namespace {

base::FilePath GetBraveVPNServicePath(const base::FilePath& target_path,
                                      const base::Version& version) {
  return target_path.AppendASCII(version.GetString())
      .Append(brave_vpn::kBraveVPNHelperExecutable);
}

bool ConfigureBraveVPNServiceAutoRestart(const base::FilePath& exe_path,
                                         const CallbackWorkItem&) {
  if (!base::PathExists(exe_path)) {
    return false;
  }
  base::CommandLine cmd(exe_path);
  cmd.AppendSwitch(brave_vpn::kBraveVpnHelperInstall);
  base::LaunchOptions options = base::LaunchOptions();
  options.wait = true;
  return base::LaunchProcess(cmd, base::LaunchOptions()).IsValid();
}

// Adds work items to register the Vpn Service with Windows. Only for
// system level installs.
void AddBraveVPNServiceWorkItems(const base::FilePath& vpn_service_path,
                                 WorkItemList* list) {
  DCHECK(::IsUserAnAdmin());

  if (vpn_service_path.empty()) {
    VLOG(1) << "The path to brave_vpn_helper.exe is invalid.";
    return;
  }
  WorkItem* install_service_work_item = new installer::InstallServiceWorkItem(
      brave_vpn::GetVpnServiceName(), brave_vpn::GetVpnServiceDisplayName(),
      SERVICE_DEMAND_START, base::CommandLine(vpn_service_path),
      base::CommandLine(base::CommandLine::NO_PROGRAM),
      brave_vpn::kBraveVpnHelperRegistryStoragePath, {}, {});
  install_service_work_item->set_best_effort(true);
  list->AddWorkItem(install_service_work_item);
  list->AddCallbackWorkItem(
      base::BindOnce(&ConfigureBraveVPNServiceAutoRestart, vpn_service_path),
      base::NullCallback());
}

}  // namespace

#define GetElevationServicePath GetElevationServicePath(target_path, new_version), install_list); \
  AddBraveVPNServiceWorkItems(GetBraveVPNServicePath
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)
#include "src/chrome/installer/setup/install_worker.cc"
#if BUILDFLAG(ENABLE_BRAVE_VPN)
#undef GetElevationServicePath
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)
#if defined(OFFICIAL_BUILD)
#undef BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION
#endif

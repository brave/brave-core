/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/installer/util/brave_shell_util.h"
#include "chrome/installer/setup/modify_params.h"
#include "chrome/installer/util/registry_util.h"
#include "chrome/installer/util/shell_util.h"
#include "chrome/installer/util/util_constants.h"
#include "chrome/installer/util/work_item.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_constants.h"
#include "brave/browser/brave_vpn/win/brave_vpn_helper/brave_vpn_helper_utils.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/install_utils.h"
#include "brave/browser/brave_vpn/win/service_details.h"
#include "brave/components/brave_vpn/browser/connection/ikev2/win/ras_utils.h"
#endif
#define UninstallProduct UninstallProduct_ChromiumImpl

#include "src/chrome/installer/setup/uninstall.cc"

#undef UninstallProduct

namespace installer {

namespace {

void DeleteBraveFileKeys(HKEY root) {
  // Delete Software\Classes\BraveXXXFile.
  std::wstring reg_prog_id(ShellUtil::kRegClasses);
  reg_prog_id.push_back(base::FilePath::kSeparators[0]);
  reg_prog_id.append(GetProgIdForFileType());
  DeleteRegistryKey(root, reg_prog_id, WorkItem::kWow64Default);

  // Cleanup OpenWithList and OpenWithProgids:
  // http://msdn.microsoft.com/en-us/library/bb166549
  std::wstring file_assoc_key;
  std::wstring open_with_progids_key;
  for (int i = 0; ShellUtil::kPotentialFileAssociations[i] != nullptr; ++i) {
    file_assoc_key.assign(ShellUtil::kRegClasses);
    file_assoc_key.push_back(base::FilePath::kSeparators[0]);
    file_assoc_key.append(ShellUtil::kPotentialFileAssociations[i]);
    file_assoc_key.push_back(base::FilePath::kSeparators[0]);

    open_with_progids_key.assign(file_assoc_key);
    open_with_progids_key.append(ShellUtil::kRegOpenWithProgids);
    if (ShouldUseFileTypeProgId(ShellUtil::kPotentialFileAssociations[i])) {
      DeleteRegistryValue(root, open_with_progids_key, WorkItem::kWow64Default,
                          GetProgIdForFileType());
    }
  }
}

}  // namespace

InstallStatus UninstallProduct(const ModifyParams& modify_params,
                               bool remove_all,
                               bool force_uninstall,
                               const base::CommandLine& cmd_line) {
  InstallStatus ret = UninstallProduct_ChromiumImpl(modify_params, remove_all,
                                                    force_uninstall, cmd_line);

  // Early return if user cancelled.
  if (ret == installer::UNINSTALL_CANCELLED) {
    return ret;
  }

  DeleteBraveFileKeys(HKEY_CURRENT_USER);

  const auto installer_state = modify_params.installer_state;
  const base::FilePath chrome_exe(
      installer_state->target_path().Append(installer::kChromeExe));
  const std::wstring suffix(
      ShellUtil::GetCurrentInstallationSuffix(chrome_exe));
  if (installer_state->system_install() ||
      (remove_all &&
       ShellUtil::QuickIsChromeRegisteredInHKLM(chrome_exe, suffix))) {
    DeleteBraveFileKeys(HKEY_LOCAL_MACHINE);
  }
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (installer_state->system_install()) {
    // TODO(bsclifton): move this to a method
    if (!InstallServiceWorkItem::DeleteService(
            brave_vpn::GetBraveVpnHelperServiceName(),
            brave_vpn::GetBraveVpnHelperRegistryStoragePath(), {}, {})) {
      LOG(WARNING) << "Failed to delete "
                   << brave_vpn::GetBraveVpnHelperServiceName();
    }

    if (!brave_vpn::UninstallBraveWireguardService() ||
        !brave_vpn::UninstallStatusTrayIcon()) {
      LOG(WARNING) << "Failed to delete "
                   << brave_vpn::GetBraveVpnWireguardServiceName();
    }
  }
  brave_vpn::ras::RemoveEntry(brave_vpn::GetBraveVPNConnectionName());
#endif

  return ret;
}

}  // namespace installer

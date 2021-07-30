/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/installer/util/brave_shell_util.h"

#define UninstallProduct UninstallProduct_ChromiumImpl

#include "../../../../../chrome/installer/setup/uninstall.cc"

#undef UninstallProduct

namespace installer {

namespace {

void DeleteBraveFileKeys(HKEY root) {
  // Delete Software\Classes\BraveXXXFile.
  std::wstring reg_prog_id(ShellUtil::kRegClasses);
  reg_prog_id.push_back(base::FilePath::kSeparators[0]);
  reg_prog_id.append(GetProgIdForFileType());
  InstallUtil::DeleteRegistryKey(root, reg_prog_id, WorkItem::kWow64Default);

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
      InstallUtil::DeleteRegistryValue(root, open_with_progids_key,
                                       WorkItem::kWow64Default,
                                       GetProgIdForFileType());
    }
  }
}

}  // namespace

InstallStatus UninstallProduct(const ModifyParams& modify_params,
                               bool remove_all,
                               bool force_uninstall,
                               const base::CommandLine& cmd_line) {
  DeleteBraveFileKeys(HKEY_CURRENT_USER);

  const InstallerState& installer_state = modify_params.installer_state;
  const base::FilePath chrome_exe(
      installer_state.target_path().Append(installer::kChromeExe));
  const std::wstring suffix(
      ShellUtil::GetCurrentInstallationSuffix(chrome_exe));
  if (installer_state.system_install() ||
      (remove_all &&
       ShellUtil::QuickIsChromeRegisteredInHKLM(chrome_exe, suffix))) {
    DeleteBraveFileKeys(HKEY_LOCAL_MACHINE);
  }

  return UninstallProduct_ChromiumImpl(modify_params, remove_all,
                                       force_uninstall, cmd_line);
}

}  // namespace installer

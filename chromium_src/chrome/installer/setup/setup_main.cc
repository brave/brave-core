/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "brave/installer/setup/archive_patch_helper.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/installer/setup/brand_behaviors.h"
#include "chrome/installer/setup/installer_state.h"
#include "chrome/installer/setup/modify_params.h"
#include "chrome/installer/setup/setup_util.h"
#include "chrome/installer/util/google_update_settings.h"
#include "chrome/installer/util/initial_preferences.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/installation_state.h"
#include "chrome/installer/util/installer_util_strings.h"
#include "chrome/installer/util/util_constants.h"

namespace {

bool BraveHandleNonInstallCmdLineOptions(
    installer::ModifyParams& modify_params,
    const base::CommandLine& cmd_line,
    const installer::InitialPreferences& prefs,
    int* exit_code) {
  if (!cmd_line.HasSwitch(installer::switches::kUpdateSetupExe)) {
    return false;
  }
  installer::InstallerState* installer_state =
      &(*modify_params.installer_state);
  const base::FilePath& setup_exe = *modify_params.setup_path;

  installer_state->SetStage(installer::UPDATING_SETUP);
  installer::InstallStatus status = installer::SETUP_PATCH_FAILED;
  // If --update-setup-exe command line option is given, we apply the given
  // patch to current exe, and store the resulting binary in the path
  // specified by --new-setup-exe. But we need to first unpack the file
  // given in --update-setup-exe.

  const base::FilePath compressed_archive(
      cmd_line.GetSwitchValuePath(installer::switches::kUpdateSetupExe));
  VLOG(1) << "Opening archive " << compressed_archive.value();
  // The top unpack failure result with 28 days aggregation (>=0.01%)
  // Setup.Install.LzmaUnPackResult_SetupExePatch
  // 0.02% PATH_NOT_FOUND
  //
  // More information can also be found with metric:
  // Setup.Install.LzmaUnPackNTSTATUS_SetupExePatch

  // We use the `new_setup_exe` directory as the working directory for
  // `ArchivePatchHelper::UncompressAndPatch`. For System installs, this
  // directory would be under %ProgramFiles% (a directory that only admins can
  // write to by default) and hence a secure location.
  const base::FilePath new_setup_exe(
      cmd_line.GetSwitchValuePath(installer::switches::kNewSetupExe));
  if (installer::ArchivePatchHelper::UncompressAndPatch(
          new_setup_exe.DirName(), compressed_archive, setup_exe,
          new_setup_exe, installer::UnPackConsumer::SETUP_EXE_PATCH)) {
    status = installer::NEW_VERSION_UPDATED;
  }

  *exit_code = InstallUtil::GetInstallReturnCode(status);
  if (*exit_code) {
    LOG(WARNING) << "setup.exe patching failed.";
    installer_state->WriteInstallerResult(status, IDS_SETUP_PATCH_FAILED_BASE,
                                          nullptr);
  }
  return true;
}

constexpr char kBraveReferralCode[] = "brave-referral-code";

void SavePromoCode(installer::InstallStatus install_status) {
  if (!InstallUtil::GetInstallReturnCode(install_status)) {
    const base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();
    if (cmd_line.HasSwitch(kBraveReferralCode)) {
      const std::string referral_code =
          cmd_line.GetSwitchValueASCII(kBraveReferralCode);
      if (!referral_code.empty()) {
        base::FilePath user_data_dir;
        base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
        base::FilePath referral_code_path =
            user_data_dir.AppendASCII("promoCode");
        if (!base::WriteFile(referral_code_path, referral_code)) {
          LOG(ERROR) << "Failed to write referral code " << referral_code
                     << " to " << referral_code_path;
        }
      }
    }
  }
}

}  // namespace

#define DoLegacyCleanups         \
  SavePromoCode(install_status); \
  DoLegacyCleanups
#define BRAVE_HANDLE_NON_INSTALL_CMD_LINE_OPTIONS                         \
  if (BraveHandleNonInstallCmdLineOptions(modify_params, cmd_line, prefs, \
                                          &exit_code)) {                  \
    return exit_code;                                                     \
  }
#define BRAVE_UPDATE_INSTALL_STATUS UpdateInstallStatus();
#define UpdateInstallStatus() \
  UpdateInstallStatus(installer_state->archive_type, install_status)
#include <chrome/installer/setup/setup_main.cc>
#undef UpdateInstallStatus
#undef BRAVE_HANDLE_NON_INSTALL_CMD_LINE_OPTIONS
#undef DoLegacyCleanups

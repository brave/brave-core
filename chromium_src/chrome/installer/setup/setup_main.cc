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
#include "chrome/common/chrome_paths.h"
#include "chrome/installer/setup/setup_util.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/util_constants.h"

namespace {

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
#include "src/chrome/installer/setup/setup_main.cc"
#undef DoLegacyCleanups

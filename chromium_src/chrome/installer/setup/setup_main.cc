/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define wWinMain wWinMain_ChromiumImpl
#include "src/chrome/installer/setup/setup_main.cc"
#undef wWinMain

const char kBraveReferralCode[] = "brave-referral-code";

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance,
                    wchar_t* command_line, int show_command) {
  int return_code = wWinMain_ChromiumImpl(instance, prev_instance, command_line,
                                          show_command);
  if (!return_code) {
    const base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();
    if (cmd_line.HasSwitch(kBraveReferralCode)) {
      const std::string referral_code =
          cmd_line.GetSwitchValueASCII(kBraveReferralCode);
      if (!referral_code.empty()) {
        base::FilePath user_data_dir;
        base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
        base::FilePath referral_code_path =
            user_data_dir.AppendASCII("promoCode");
        if (!base::WriteFile(referral_code_path, referral_code.c_str(),
                             referral_code.size())) {
          LOG(ERROR) << "Failed to write referral code " << referral_code
                     << " to " << referral_code_path;
        }
      }
    }
  }

  return return_code;
}

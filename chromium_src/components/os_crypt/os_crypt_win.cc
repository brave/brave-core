/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "brave/common/brave_switches.h"

// switches::kDisableEncryptionWin
const char kDisableEncryptionWin[] = "disable-encryption-win";

namespace {
bool IsEncryptionDisabled(const std::string& input_text,
                          std::string* output_text) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          kDisableEncryptionWin)) {
    *output_text = input_text;
    return true;
  }
  return false;
}

}  // namespace
#include "../../../../components/os_crypt/os_crypt_win.cc"

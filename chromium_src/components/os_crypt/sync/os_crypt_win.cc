/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "brave/components/constants/brave_switches.h"

namespace {
bool IsEncryptionDisabled(const std::string& input_text,
                          std::string* output_text) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableEncryptionWin)) {
    *output_text = input_text;
    return true;
  }
  return false;
}

}  // namespace
#include "src/components/os_crypt/sync/os_crypt_win.cc"

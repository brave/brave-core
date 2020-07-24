/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/process/launch.h"

namespace {
void BraveLaunchOption(base::CommandLine* cmd_line,
                       base::LaunchOptions *options) {
  // tor::swtiches::kTorExecutablePath
  if (cmd_line->HasSwitch("tor-executable-path"))
    options->start_hidden = true;
}

}  // namespace
#include "../../../../../sandbox/policy/win/sandbox_win.cc"

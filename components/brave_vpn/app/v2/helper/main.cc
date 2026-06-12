/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/process/memory.h"
#include "brave/components/brave_vpn/app/v2/helper/helper_app.h"
#include "brave/components/brave_vpn/app/v2/shared/app_utils.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include "base/win/process_startup_helper.h"
#endif

int main(int argc, char* argv[]) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);
  const auto& command_line = *base::CommandLine::ForCurrentProcess();

  base::EnableTerminationOnHeapCorruption();
  base::EnableTerminationOnOutOfMemory();
#if BUILDFLAG(IS_WIN)
  base::win::RegisterInvalidParamHandler();
  base::win::SetupCRT(command_line);
#endif

  brave_vpn::v2::app_utils::InitLogging(command_line);

  brave_vpn::v2::HelperApp helper_app;
  return helper_app.Run();
}

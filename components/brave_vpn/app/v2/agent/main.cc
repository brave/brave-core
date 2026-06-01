/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/process/memory.h"
#include "brave/components/brave_vpn/app/v2/agent/agent_app.h"
#include "build/build_config.h"

namespace {
inline constexpr char kVpnAppLogFile[] = "log-file";

void InitLogging(const base::CommandLine& command_line) {
  logging::LoggingSettings settings;
  settings.logging_dest =
      logging::LOG_TO_SYSTEM_DEBUG_LOG | logging::LOG_TO_STDERR;
  base::FilePath log_file_path;
  if (command_line.HasSwitch(kVpnAppLogFile)) {
    settings.logging_dest |= logging::LOG_TO_FILE;
    log_file_path = command_line.GetSwitchValuePath(kVpnAppLogFile);
    settings.log_file_path = log_file_path.value().c_str();
  }
  logging::InitLogging(settings);
}
}  // namespace

#if BUILDFLAG(IS_WIN)
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, wchar_t*, int) {
#else
int main(int argc, char* argv[]) {
#endif
  base::AtExitManager exit_manager;
#if BUILDFLAG(IS_WIN)
  base::CommandLine::Init(0, nullptr);
#else
  base::CommandLine::Init(argc, argv);
#endif
  const auto& command_line = *base::CommandLine::ForCurrentProcess();

  base::EnableTerminationOnHeapCorruption();
  base::EnableTerminationOnOutOfMemory();
#if BUILDFLAG(IS_WIN)
  base::win::RegisterInvalidParamHandler();
  base::win::SetupCRT(command_line);
#endif

  InitLogging(command_line);

  brave_vpn::v2::AgentApp agent_app;
  return agent_app.Run();
}

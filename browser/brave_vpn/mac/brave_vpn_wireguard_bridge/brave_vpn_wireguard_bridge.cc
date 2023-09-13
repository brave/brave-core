/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "brave/browser/brave_vpn/mac/brave_vpn_wireguard_bridge/brave_vpn_runner_mac.h"

namespace {
const char kConfigPathSwitchName[] = "config-path";
const char kRemoveSwitchName[] = "remove";
const char kLogFile[] = "log-file";
}  // namespace

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  auto* command_line = base::CommandLine::ForCurrentProcess();
  logging::LoggingSettings settings;
  settings.logging_dest =
      logging::LOG_TO_SYSTEM_DEBUG_LOG | logging::LOG_TO_STDERR;
  base::FilePath log_file_path;
  if (command_line->HasSwitch(kLogFile)) {
    settings.logging_dest |= logging::LOG_TO_FILE;
    log_file_path = command_line->GetSwitchValuePath(kLogFile);
    settings.log_file_path = log_file_path.value().c_str();
  }
  logging::InitLogging(settings);

  if (command_line->HasSwitch(kConfigPathSwitchName)) {
    auto config_path = command_line->GetSwitchValuePath(kConfigPathSwitchName);
    if (!base::PathExists(config_path)) {
      LOG(ERROR) << "Config file not found:" << config_path;
      return 1;
    }
    std::string config;
    if (!base::ReadFileToString(config_path, &config)) {
      LOG(ERROR) << "Unable to read config file:" << config_path;
      return 1;
    }
    LOG(ERROR) << __func__;
    return brave_vpn::BraveVpnRunnerMac::GetInstance()->EnableVPN(config);
  }
  if (command_line->HasSwitch(kRemoveSwitchName)) {
    return brave_vpn::BraveVpnRunnerMac::GetInstance()->RemoveVPN();
  }
  return 0;
}

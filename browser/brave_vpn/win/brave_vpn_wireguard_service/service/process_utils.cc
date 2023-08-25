/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/process_utils.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/process/process_handle.h"
#include "base/process/process_iterator.h"
#include "base/win/windows_types.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_constants.h"

namespace brave_vpn {

namespace {

constexpr wchar_t kWindowsExplorerExecutableName[] = L"EXPLORER.EXE";

void RunCommandForUser(base::UserTokenHandle token,
                       const std::string& command) {
  base::FilePath exe_dir;
  base::PathService::Get(base::DIR_EXE, &exe_dir);
  base::CommandLine cmd(
      exe_dir.Append(brave_vpn::kBraveVpnWireguardServiceExecutable));
  cmd.AppendSwitch(command);
  base::LaunchOptions options;
  options.as_user = token;
  if (!base::LaunchProcess(cmd, options).IsValid()) {
    VLOG(1) << "Interactive process launch failed";
  }
}

}  // namespace

// Looking for explorer.exe to extract user token and launch tray process
// to setup Brave VPN tray icon.
void RunWireGuardCommandForUsers(const std::string& command) {
  VLOG(1) << __func__;
  base::NamedProcessIterator iter(kWindowsExplorerExecutableName, nullptr);
  while (const base::ProcessEntry* process_entry = iter.NextProcessEntry()) {
    base::win::ScopedHandle process(
        ::OpenProcess(PROCESS_QUERY_INFORMATION, false, process_entry->pid()));
    if (!process.is_valid()) {
      continue;
    }
    base::win::ScopedHandle user_token_handle;
    base::UserTokenHandle user_token;
    if (!OpenProcessToken(process.Get(), TOKEN_ALL_ACCESS, &user_token)) {
      continue;
    }
    user_token_handle.Set(user_token);

    RunCommandForUser(user_token_handle.Get(), command);
  }
}

}  // namespace brave_vpn

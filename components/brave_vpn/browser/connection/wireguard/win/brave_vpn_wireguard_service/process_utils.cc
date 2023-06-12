/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/process_utils.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/process/process_handle.h"
#include "base/process/process_iterator.h"
#include "base/win/windows_types.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/common/service_constants.h"

namespace brave_vpn {

namespace {

constexpr wchar_t kWindowsExplorerExecutableName[] = L"EXPLORER.EXE";

void LaunchInteractiveProcessAsUser(base::UserTokenHandle token) {
  base::FilePath exe_dir;
  base::PathService::Get(base::DIR_EXE, &exe_dir);
  base::CommandLine interactive_cmd(
      exe_dir.Append(brave_vpn::kBraveVpnWireguardServiceExecutable));
  interactive_cmd.AppendSwitch(
      brave_vpn::kBraveVpnWireguardServiceInteractiveSwitchName);
  base::LaunchOptions options;
  options.as_user = token;
  options.empty_desktop_name = true;
  if (!base::LaunchProcess(interactive_cmd, options).IsValid()) {
    VLOG(1) << "Interactive process launch failed";
  }
}

}  // namespace

// Looking for explorer.exe to extract user token and launch interactive process
// to setup Brave VPN tray icon.
void RunInteractiveProcess() {
  VLOG(1) << __func__;
  base::NamedProcessIterator iter(kWindowsExplorerExecutableName, nullptr);
  while (const base::ProcessEntry* process_entry = iter.NextProcessEntry()) {
    base::win::ScopedHandle process(::OpenProcess(PROCESS_QUERY_INFORMATION, false, process_entry->pid()));
    if (!process.is_valid()) {
      continue;
    }
    base::win::ScopedHandle user_token_handle;
    base::UserTokenHandle user_token;
    if (!OpenProcessToken(process.Get(), TOKEN_ALL_ACCESS, &user_token)) {
      continue;
    }
    user_token_handle.Set(user_token);

    LaunchInteractiveProcessAsUser(user_token_handle.Get());
  }
}

}  // namespace brave_vpn

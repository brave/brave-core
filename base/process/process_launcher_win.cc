/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/base/process/process_launcher.h"

#include <optional>
#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/notreached.h"
#include "base/process/launch.h"
#include "base/win/scoped_handle.h"
#include "base/win/scoped_process_information.h"

namespace brave {
ProcessLauncher::ProcessLauncher() = default;
ProcessLauncher::~ProcessLauncher() = default;

// static
std::optional<std::string> ProcessLauncher::ReadAppOutput(
    base::CommandLine cmdline,
    base::LaunchOptions options,
    int timeout_sec) {
  HANDLE out_read = nullptr;
  HANDLE out_write = nullptr;

  SECURITY_ATTRIBUTES sa_attr;
  // Set the bInheritHandle flag so pipe handles are inherited.
  sa_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa_attr.bInheritHandle = TRUE;
  sa_attr.lpSecurityDescriptor = nullptr;

  // Create the pipe for the child process's STDOUT.
  if (!CreatePipe(&out_read, &out_write, &sa_attr, 0)) {
    return std::nullopt;
  }

  // Ensure we don't leak the handles.
  base::win::ScopedHandle scoped_out_read(out_read);
  base::win::ScopedHandle scoped_out_write(out_write);

  // Ensure the read handles to the pipes are not inherited.
  if (!SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0)) {
    return std::nullopt;
  }

  options.stdin_handle = out_read;
  options.stdout_handle = out_write;
  options.stderr_handle = out_read;
  options.inherit_mode = base::LaunchOptions::Inherit::kAll;

  base::Process process = base::LaunchProcess(cmdline, options);
  if (!process.IsValid()) {
    return std::nullopt;
  }

  bool exited = false;
  int exit_code = 0;
  std::string result;
  scoped_out_write.Close();

  bool read_result = base::ReadStreamToString(
      base::FileToFILE(base::File(std::move(scoped_out_read)), "r"), &result);

  base::ScopedAllowBaseSyncPrimitives allow_wait_for_process;
  exited =
      process.WaitForExitWithTimeout(base::Seconds(timeout_sec), &exit_code);
  if (!exited) {
    process.Terminate(0, true);
  }
  if (exited && !exit_code && read_result) {
    return result;
  } else {
    return std::nullopt;
  }
}

}  // namespace brave

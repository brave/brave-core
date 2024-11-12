/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/base/process/process_launcher.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/process/launch.h"

namespace brave {

ProcessLauncher::ProcessLauncher() = default;
ProcessLauncher::~ProcessLauncher() = default;

// static
std::optional<std::string> ProcessLauncher::ReadAppOutput(
    base::CommandLine cmdline,
    base::LaunchOptions options,
    int timeout_sec) {
  std::array<int, 2> pipe_fd;
  if (pipe(pipe_fd) < 0) {
    return std::nullopt;
  }

  options.fds_to_remap.emplace_back(pipe_fd[1], STDOUT_FILENO);
  options.fds_to_remap.emplace_back(pipe_fd[1], STDERR_FILENO);

  base::Process process = base::LaunchProcess(cmdline, options);
  if (!process.IsValid()) {
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    return std::nullopt;
  }

  bool exited = false;
  int exit_code = 0;
  std::string result;
  close(pipe_fd[1]);
  bool read_result = base::ReadStreamToString(
      base::FileToFILE(base::File(pipe_fd[0]), "r"), &result);
  close(pipe_fd[0]);

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

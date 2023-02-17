/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/base/process/process_launcher.h"

#include <string>
#include <vector>

#include "base/posix/eintr_wrapper.h"
#include "base/process/launch.h"

namespace brave {
namespace {
bool ReadFdToString(int fd, std::string* ret) {
  DCHECK(ret);
  ret->clear();

  ssize_t bytes_read = 0;
  do {
    char buf[4096];
    bytes_read = HANDLE_EINTR(read(fd, buf, sizeof(buf)));
    if (bytes_read < 0) {
      return false;
    }
    if (bytes_read > 0) {
      ret->append(buf, static_cast<size_t>(bytes_read));
    }
  } while (bytes_read > 0);

  return true;
}

}  // namespace

ProcessLauncher::ProcessLauncher() = default;
ProcessLauncher::~ProcessLauncher() = default;

// static
absl::optional<std::string> ProcessLauncher::ReadAppOutput(
    base::CommandLine cmdline,
    base::LaunchOptions options,
    int timeout_sec) {
  int pipe_fd[2];
  if (pipe(pipe_fd) < 0) {
    return absl::nullopt;
  }

  options.fds_to_remap.emplace_back(pipe_fd[1], STDOUT_FILENO);
  options.fds_to_remap.emplace_back(pipe_fd[1], STDERR_FILENO);

  base::Process process = base::LaunchProcess(cmdline, options);
  if (!process.IsValid()) {
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    return absl::nullopt;
  }

  bool exited = false;
  int exit_code = 0;
  std::string result;
  close(pipe_fd[1]);
  ReadFdToString(pipe_fd[0], &result);
  close(pipe_fd[0]);

  base::ScopedAllowBaseSyncPrimitives allow_wait_for_process;
  exited =
      process.WaitForExitWithTimeout(base::Seconds(timeout_sec), &exit_code);
  if (!exited) {
    process.Terminate(0, true);
  }
  if (exited && !exit_code) {
    return result;
  } else {
    return absl::nullopt;
  }
}

}  // namespace brave

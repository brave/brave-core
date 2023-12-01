/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BASE_PROCESS_PROCESS_LAUNCHER_H_
#define BRAVE_BASE_PROCESS_PROCESS_LAUNCHER_H_

#include <optional>
#include <string>

#include "base/process/launch.h"
#include "base/process/process.h"

namespace brave {
class ProcessLauncher {
  ProcessLauncher();
  ~ProcessLauncher();

 public:
  /**
   * Launches process in this thread and reads the output.
   * This works like GetAppOutput, but respects provided LaunchOptions.
   */
  static std::optional<std::string> ReadAppOutput(base::CommandLine cmdline,
                                                  base::LaunchOptions options,
                                                  int timeout_sec);
};

}  // namespace brave

#endif  // BRAVE_BASE_PROCESS_PROCESS_LAUNCHER_H_

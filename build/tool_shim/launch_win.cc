/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/build/tool_shim/launch.h"

#include <windows.h>

#include "base/command_line.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/strings/strcat.h"

namespace {

// Same approach as redirect_cc: quote each argv entry for CommandLineToArgvW
// and join into a single command-line string for LaunchProcess.
base::CommandLine::StringType CreateCmdLine(
    const std::vector<base::FilePath::StringType>& argv) {
  std::vector<base::CommandLine::StringType> quoted_args;
  quoted_args.reserve(argv.size());
  for (const auto& arg : argv) {
    quoted_args.push_back(base::CommandLine::QuoteForCommandLineToArgvW(arg));
  }
  return base::JoinString(quoted_args, FILE_PATH_LITERAL(" "));
}

}  // namespace

base::expected<int, std::string> Launch(const LaunchConfig& config) {
  if (config.argv.empty()) {
    return base::unexpected("Launch requires argv[0] as the program path.");
  }

  // Create the launch options.
  base::LaunchOptions options;
  options.current_directory = config.cwd;
  options.environment.insert(config.env.begin(), config.env.end());

  // Launch the process.
  base::Process process =
      base::LaunchProcess(CreateCmdLine(config.argv), options);
  if (!process.IsValid()) {
    return base::unexpected("base::LaunchProcess failed");
  }

  int exit_code = -1;
  if (!process.WaitForExit(&exit_code)) {
    return base::unexpected("Failed to WaitForExit");
  }
  return exit_code;
}

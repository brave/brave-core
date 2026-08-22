/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "brave/build/tool_shim/launch.h"
#include "brave/build/tool_shim/tool_shim_config.h"
#include "brave/build/tool_shim/utils.h"

// sysexits.h: EX_CONFIG (Configuration error).
constexpr int kExitInvalidConfig = 78;
// POSIX standard: Command cannot execute.
constexpr int kExitCannotExecute = 126;

int main(int argc, char* argv[]) {
  // Initialize the command line.
  base::CommandLine::Init(argc, argv);

  // Configure logging.
  ConfigureLogging();

  // Get the executable path and directory.
  const base::FilePath exe_path = base::PathService::CheckedGet(base::FILE_EXE);
  const base::FilePath exe_dir = exe_path.DirName();

  // Parse the tool shim config.
  base::expected<ToolShimConfig, std::string> config =
      ParseToolShimConfig(exe_path.ReplaceExtension(FILE_PATH_LITERAL("json")));
  if (!config.has_value()) {
    LOG(ERROR) << config.error();
    return kExitInvalidConfig;
  }

  // Log the tool shim config.
  VLOG(1) << *config;

  // Initialize the launch config.
  LaunchConfig launch_config;

  // Fill in the executable and arguments.
  launch_config.argv.reserve(1 + config->args.size());
  launch_config.argv.push_back(
      ResolveAgainst(exe_dir, config->executable).value());
  launch_config.argv.insert(launch_config.argv.end(), config->args.begin(),
                            config->args.end());

  // Fill in the remaining arguments.
  for (const auto& arg :
       base::span(base::CommandLine::ForCurrentProcess()->argv()).subspan(1u)) {
    launch_config.argv.push_back(arg);
  }

  // Fill in the cwd.
  if (config->cwd) {
    launch_config.cwd = ResolveAgainst(exe_dir, *config->cwd);
  } else {
    launch_config.cwd = exe_dir;
  }

  // Fill in the environment variables.
  launch_config.env = config->env;

  // Log the launch config.
  VLOG(1) << launch_config;

  // Launch the target executable.
  base::expected<int, std::string> exit_code = Launch(launch_config);
  if (!exit_code.has_value()) {
    // Use PLOG to also log the system error message on failed launch.
    PLOG(ERROR) << exit_code.error() << "\n" << launch_config << "\n";
    return kExitCannotExecute;
  }

  // Return the exit code of the target executable.
  return *exit_code;
}

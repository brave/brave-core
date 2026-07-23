/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/build/tool_shim/launch.h"

#include <unistd.h>

#include <vector>

#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/strings/strcat.h"

base::expected<int, std::string> Launch(const LaunchConfig& config) {
  if (config.argv.empty()) {
    return base::unexpected("Launch requires argv[0] as the program path.");
  }

  // Set the current working directory.
  if (!base::SetCurrentDirectory(config.cwd)) {
    return base::unexpected(
        base::StrCat({"Failed to set cwd to: ", config.cwd.AsUTF8Unsafe()}));
  }

  // Set the environment variables.
  if (!config.env.empty()) {
    auto environment = base::Environment::Create();
    for (const auto& [key, val] : config.env) {
      if (!environment->SetVar(key, val)) {
        return base::unexpected(
            base::StrCat({"Failed to set env var: ", key, "=", val}));
      }
    }
  }

  // Create the exec argv.
  std::vector<char*> exec_argv;
  exec_argv.reserve(config.argv.size() + 1);
  for (auto& arg : config.argv) {
    exec_argv.push_back(const_cast<char*>(arg.c_str()));
  }
  exec_argv.push_back(nullptr);

  // Execute the program.
  execvp(config.argv[0].c_str(), exec_argv.data());
  // After this point, the process will be replaced by the new program.

  // If we reach this point, execvp failed.
  return base::unexpected("execvp failed");
}

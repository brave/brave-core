/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BUILD_TOOL_SHIM_LAUNCH_H_
#define BRAVE_BUILD_TOOL_SHIM_LAUNCH_H_

#include <iosfwd>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/types/expected.h"

struct LaunchConfig {
  std::vector<base::FilePath::StringType> argv;
  base::FilePath cwd;
  base::flat_map<base::FilePath::StringType, base::FilePath::StringType> env;
};

// Launches |argv[0]| as the program with the remaining entries as argv,
// using |cwd| and |env| overrides. Returns the child exit code, or an error
// if the process could not be launched. On POSIX this replaces the current
// process via execvp (only returns on failure).
base::expected<int, std::string> Launch(const LaunchConfig& config);

std::ostream& operator<<(std::ostream& os, const LaunchConfig& config);

#endif  // BRAVE_BUILD_TOOL_SHIM_LAUNCH_H_

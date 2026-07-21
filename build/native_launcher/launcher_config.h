/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BUILD_NATIVE_LAUNCHER_LAUNCHER_CONFIG_H_
#define BRAVE_BUILD_NATIVE_LAUNCHER_LAUNCHER_CONFIG_H_

#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/types/expected.h"

struct LauncherConfig {
  base::FilePath::StringType executable;
  std::vector<base::FilePath::StringType> args;
  base::flat_map<base::FilePath::StringType, base::FilePath::StringType> env;
  std::optional<base::FilePath::StringType> cwd;
};

// Reads and parses the sidecar JSON at |json_path|. On failure returns an
// error message. String fields are converted to the OS native string type;
// paths remain unresolved relative/absolute strings from JSON.
base::expected<LauncherConfig, std::string> ParseLauncherConfig(
    const base::FilePath& json_path);

#endif  // BRAVE_BUILD_NATIVE_LAUNCHER_LAUNCHER_CONFIG_H_

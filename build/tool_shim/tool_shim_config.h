/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BUILD_TOOL_SHIM_TOOL_SHIM_CONFIG_H_
#define BRAVE_BUILD_TOOL_SHIM_TOOL_SHIM_CONFIG_H_

#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/types/expected.h"

struct ToolShimConfig {
  base::FilePath::StringType executable;
  std::vector<base::FilePath::StringType> args;
  base::flat_map<base::FilePath::StringType, base::FilePath::StringType> env;
  std::optional<base::FilePath::StringType> cwd;

  // Debug fields for logging, set only if VLOG(1) is enabled.
  base::FilePath json_path;
  std::string json_text;
};

// Reads and parses the sidecar JSON at |json_path|. On failure returns an
// error message. String fields are converted to the OS native string type;
// paths remain unresolved relative/absolute strings from JSON.
base::expected<ToolShimConfig, std::string> ParseToolShimConfig(
    const base::FilePath& json_path);

std::ostream& operator<<(std::ostream& os, const ToolShimConfig& config);

#endif  // BRAVE_BUILD_TOOL_SHIM_TOOL_SHIM_CONFIG_H_

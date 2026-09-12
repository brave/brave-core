/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BUILD_TOOL_SHIM_UTILS_H_
#define BRAVE_BUILD_TOOL_SHIM_UTILS_H_

#include <string_view>

#include "base/files/file_path.h"

// Resolves |path| against |base_dir| when relative, then normalizes via
// base::MakeAbsoluteFilePath (path must exist). Must be done before changing
// the process cwd.
base::FilePath ResolveAgainst(const base::FilePath& base_dir,
                              const base::FilePath::StringType& path);

// If TOOL_SHIM_VERBOSE=1 is set in the environment, enables verbose logging.
void ConfigureLogging();

// Converts a UTF-8 string to a native string.
base::FilePath::StringType ToNativeString(std::string_view value);

#endif  // BRAVE_BUILD_TOOL_SHIM_UTILS_H_

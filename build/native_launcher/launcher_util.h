/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BUILD_NATIVE_LAUNCHER_LAUNCHER_UTIL_H_
#define BRAVE_BUILD_NATIVE_LAUNCHER_LAUNCHER_UTIL_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/types/expected.h"

// Resolves |path| against |base_dir| when relative, then normalizes via
// base::MakeAbsoluteFilePath (path must exist). Must be done before changing
// the process cwd.
base::FilePath ResolveAgainst(const base::FilePath& base_dir,
                              const base::FilePath::StringType& path);

// If NATIVE_LAUNCHER_VERBOSE=1 is set in the environment, prints the launch
// command, cwd, and env overrides to stderr.
void MaybeLogVerboseLaunch(
    const std::vector<base::FilePath::StringType>& args,
    const base::FilePath& cwd,
    const base::flat_map<base::FilePath::StringType,
                         base::FilePath::StringType>& env);

// Launches |args[0]| as the program with the remaining entries as argv,
// using |cwd| and |env| overrides. Returns the child exit code, or an error
// if the process could not be launched. On POSIX this replaces the current
// process via execvp (only returns on failure).
base::expected<int, std::string> Launch(
    std::vector<base::FilePath::StringType> args,
    const base::FilePath& cwd,
    const base::flat_map<base::FilePath::StringType,
                         base::FilePath::StringType>& env);

#endif  // BRAVE_BUILD_NATIVE_LAUNCHER_LAUNCHER_UTIL_H_

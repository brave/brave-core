/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/build/native_launcher/launcher_util.h"

#include <iostream>

#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"

base::FilePath ResolveAgainst(const base::FilePath& base_dir,
                              const base::FilePath::StringType& path) {
  const base::FilePath file_path(path);
  const base::FilePath resolved =
      file_path.IsAbsolute() ? file_path : base_dir.Append(file_path);
  // Assumes |resolved| exists; expands symlinks and collapses "." / "..".
  return base::MakeAbsoluteFilePath(resolved);
}

void MaybeLogVerboseLaunch(
    const std::vector<base::FilePath::StringType>& args,
    const base::FilePath& cwd,
    const base::flat_map<base::FilePath::StringType,
                         base::FilePath::StringType>& env) {
  auto environment = base::Environment::Create();
  if (environment->GetVar("NATIVE_LAUNCHER_VERBOSE").value_or("") != "1") {
    return;
  }

  const auto log = [](base::FilePath::StringViewType str) {
#if BUILDFLAG(IS_WIN)
    std::wcerr
#else
    std::cerr
#endif  // BUILDFLAG(IS_WIN)
        << str << FILE_PATH_LITERAL("\n");
  };

  log(FILE_PATH_LITERAL("--- native launcher start ---"));
  log(base::StrCat({FILE_PATH_LITERAL("cwd: "), cwd.value()}));
  log(base::StrCat({FILE_PATH_LITERAL("cmd: "),
                    base::JoinString(args, FILE_PATH_LITERAL(" "))}));
  if (!env.empty()) {
    log(FILE_PATH_LITERAL("env:"));
    for (const auto& [key, val] : env) {
      log(base::StrCat(
          {FILE_PATH_LITERAL("  "), key, FILE_PATH_LITERAL("="), val}));
    }
  }
  log(FILE_PATH_LITERAL("--- native launcher end ---"));
}

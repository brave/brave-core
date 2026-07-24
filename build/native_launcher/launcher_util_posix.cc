/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/build/native_launcher/launcher_util.h"

#include <unistd.h>

#include <vector>

#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/strings/strcat.h"

base::expected<int, std::string> Launch(
    std::vector<base::FilePath::StringType> args,
    const base::FilePath& cwd,
    const base::flat_map<base::FilePath::StringType,
                         base::FilePath::StringType>& env) {
  if (args.empty()) {
    return base::unexpected("Launch requires args[0] as the program path.");
  }

  if (!base::SetCurrentDirectory(cwd)) {
    return base::unexpected(
        base::StrCat({"Failed to set cwd to: ", cwd.AsUTF8Unsafe()}));
  }

  if (!env.empty()) {
    auto environment = base::Environment::Create();
    for (const auto& [key, val] : env) {
      if (!environment->SetVar(key, val)) {
        return base::unexpected(base::StrCat({"Failed to set env var: ", key}));
      }
    }
  }

  std::vector<char*> exec_argv;
  exec_argv.reserve(args.size() + 1);
  for (auto& arg : args) {
    exec_argv.push_back(const_cast<char*>(arg.c_str()));
  }
  exec_argv.push_back(nullptr);

  execvp(args[0].c_str(), exec_argv.data());
  return base::unexpected(base::StrCat({"execvp failed to launch: ", args[0]}));
}

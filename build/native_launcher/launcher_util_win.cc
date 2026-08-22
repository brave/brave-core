/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/build/native_launcher/launcher_util.h"

#include <windows.h>

#include "base/command_line.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"

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

base::expected<int, std::string> Launch(
    std::vector<base::FilePath::StringType> args,
    const base::FilePath& cwd,
    const base::flat_map<base::FilePath::StringType,
                         base::FilePath::StringType>& env) {
  if (args.empty()) {
    return base::unexpected("Launch requires args[0] as the program path.");
  }

  base::LaunchOptions options;
  options.current_directory = cwd;
  options.environment.insert(env.begin(), env.end());

  base::Process process = base::LaunchProcess(CreateCmdLine(args), options);
  if (!process.IsValid()) {
    return base::unexpected(
        base::StrCat({"base::LaunchProcess failed to launch: ",
                      base::FilePath(args[0]).AsUTF8Unsafe()}));
  }

  int exit_code = -1;
  if (!process.WaitForExit(&exit_code)) {
    return base::unexpected("Failed to WaitForExit");
  }
  return exit_code;
}

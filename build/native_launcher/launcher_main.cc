/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "brave/build/native_launcher/launcher_config.h"
#include "brave/build/native_launcher/launcher_exit_codes.h"
#include "brave/build/native_launcher/launcher_util.h"
#include "build/build_config.h"

int main(int argc, char* argv[]) {
#if BUILDFLAG(IS_WIN)
  // On Windows, Init reads GetCommandLineW() and ignores these arguments.
  base::CommandLine::Init(0, nullptr);
#else
  base::CommandLine::Init(argc, argv);
#endif

  const base::FilePath exe_path = base::PathService::CheckedGet(base::FILE_EXE);
  const base::FilePath exe_dir = exe_path.DirName();

  base::expected<LauncherConfig, std::string> config =
      ParseLauncherConfig(exe_path.ReplaceExtension(FILE_PATH_LITERAL("json")));
  if (!config.has_value()) {
    LOG(ERROR) << config.error();
    return LauncherExitCode::kExitInvalidConfig;
  }

  // Resolve executable against exe_dir before applying cwd.
  std::vector<base::FilePath::StringType> final_args;
  final_args.reserve(1 + config->args.size());
  final_args.push_back(ResolveAgainst(exe_dir, config->executable).value());
  final_args.insert(final_args.end(), config->args.begin(), config->args.end());

  for (const auto& arg :
       base::span(base::CommandLine::ForCurrentProcess()->argv()).subspan(1u)) {
    final_args.push_back(arg);
  }

  base::FilePath resolved_cwd = exe_dir;
  if (config->cwd) {
    resolved_cwd = ResolveAgainst(exe_dir, *config->cwd);
  }

  MaybeLogVerboseLaunch(final_args, resolved_cwd, config->env);

  base::expected<int, std::string> exit_code =
      Launch(std::move(final_args), resolved_cwd, config->env);
  if (!exit_code.has_value()) {
    LOG(ERROR) << exit_code.error();
    return LauncherExitCode::kExitCannotExecute;
  }
  return exit_code.value();
}

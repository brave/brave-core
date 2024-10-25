/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/constants.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "components/component_updater/component_updater_paths.h"

#if BUILDFLAG(IS_WIN)
#include "base/command_line.h"
#endif  // BUILDFLAG(IS_WIN)

namespace tor {

namespace {

// The filename for the tor client config file.
constexpr base::FilePath::StringPieceType kTorRcFilename =
    FILE_PATH_LITERAL("tor-torrc");

base::FilePath GetUserDataDir() {
#if BUILDFLAG(IS_WIN)
  // The switch used to set a custom user data dir.
  constexpr char kUserDataDir[] = "user-data-dir";
  // It is not very clear why, but only on windows `PathService` user dir wasn't
  // getting overridden when passing `kUserDataDir`, so we only check for the
  // switch on Windows where the problem occurs.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  base::FilePath user_data_dir = command_line->GetSwitchValuePath(kUserDataDir);
  if (!user_data_dir.empty()) {
    return user_data_dir;
  }
#endif  // BUILDFLAG(IS_WIN)
  return base::PathService::CheckedGet(component_updater::DIR_COMPONENT_USER);
}

}  // namespace

base::FilePath GetTorClientDirectory() {
  return GetUserDataDir().AppendASCII(kTorClientComponentId);
}

base::FilePath GetClientExecutablePath(const base::SafeBaseName& install_dir,
                                       const base::SafeBaseName& executable) {
  return GetTorClientDirectory()
      .Append(install_dir.path())
      .Append(executable.path());
}

base::FilePath GetTorRcPath(const base::SafeBaseName& install_dir) {
  return GetTorClientDirectory()
      .Append(install_dir.path())
      .Append(kTorRcFilename);
}

base::FilePath GetTorDataPath() {
  return GetUserDataDir()
      .Append(FILE_PATH_LITERAL("tor"))
      .Append(FILE_PATH_LITERAL("data"));
}

base::FilePath GetTorWatchPath() {
  return GetUserDataDir()
      .Append(FILE_PATH_LITERAL("tor"))
      .Append(FILE_PATH_LITERAL("watch"));
}

}  // namespace tor

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/nix/xdg_util.h"
#include "brave/common/brave_channel_info_posix.h"
#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "chrome/common/chrome_paths_internal.h"

namespace {

// Brave doesn't use CHROME_CONFIG_HOME or Google Chrome's directory names; it
// always lives under BraveSoftware, with a channel-specific suffix and a
// separate name when Brave Origin branded.
bool BraveGetDefaultUserDataDirectory(base::FilePath* result) {
  auto env = base::Environment::Create();
  base::FilePath config_dir = base::nix::GetXDGDirectory(
      env.get(), base::nix::kXdgConfigHomeEnvVar, base::nix::kDotConfigDir);

  std::string data_dir_suffix;
  brave::GetChannelImpl(nullptr, &data_dir_suffix);

#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  *result = config_dir.Append("BraveSoftware/Brave-Origin" + data_dir_suffix);
#else
  *result = config_dir.Append("BraveSoftware/Brave-Browser" + data_dir_suffix);
#endif
  return true;
}

}  // namespace

#include <chrome/common/chrome_paths_linux.cc>

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/shared/app_utils.h"

#include "base/apple/bundle_locations.h"
#include "base/apple/foundation_util.h"
#include "base/files/file_enumerator.h"
#include "base/logging.h"
#include "base/path_service.h"

namespace brave_vpn::v2::app_utils {
namespace {
inline constexpr base::FilePath::CharType kFrameworkExtension[] =
    FILE_PATH_LITERAL(".framework");
}  // namespace

base::FilePath ResolveFrameworkPath(const base::FilePath& exe_dir,
                                    bool am_i_bundled) {
  base::FilePath framework_path;
  if (am_i_bundled) {
    // Bundled app: the executable is nested inside the framework.
    base::FilePath path = exe_dir;
    while (!path.empty() &&
           path.BaseName().Extension() != kFrameworkExtension) {
      base::FilePath parent = path.DirName();
      if (parent == path) {
        return base::FilePath();
      }
      path = parent;
    }
    framework_path = path;
  } else {
    // Standalone binary: the framework sits beside it.
    base::FileEnumerator e(exe_dir, /*recursive=*/false,
                           base::FileEnumerator::DIRECTORIES);
    for (base::FilePath checked_path = e.Next(); !checked_path.empty();
         checked_path = e.Next()) {
      if (checked_path.BaseName().Extension() == kFrameworkExtension) {
        framework_path = checked_path;
        break;
      }
    }
  }
  return framework_path;
}

void ConfigureFrameworkBundlePath() {
  base::FilePath framework_path = ResolveFrameworkPath(
      base::PathService::CheckedGet(base::DIR_EXE), base::apple::AmIBundled());
  if (!framework_path.empty()) {
    DVLOG(1) << "Framework detected: " << framework_path;
    base::apple::SetOverrideFrameworkBundlePath(framework_path);
  }
}

}  // namespace brave_vpn::v2::app_utils

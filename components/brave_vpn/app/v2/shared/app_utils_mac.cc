/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/shared/app_utils.h"

#include "base/apple/bundle_locations.h"
#include "base/apple/foundation_util.h"
#include "base/files/file_enumerator.h"
#include "base/path_service.h"

namespace brave_vpn {
namespace v2 {
namespace app_utils {

void ConfigureFrameworkBundlePath() {
  base::FilePath exe_path = base::PathService::CheckedGet(base::DIR_EXE);
  base::FilePath framework_path;

  if (base::apple::AmIBundled()) {
    // Bundled app: nested inside the framework.
    framework_path = exe_path;
    while (!framework_path.empty() &&
           framework_path.BaseName().Extension() != ".framework") {
      framework_path = framework_path.DirName();
    }
  } else {
    // Standalone binary: the framework sits beside it.
    base::FileEnumerator e(exe_path, /*recursive=*/false,
                           base::FileEnumerator::DIRECTORIES);
    for (base::FilePath checked_path = e.Next(); !checked_path.empty();
         checked_path = e.Next()) {
      if (checked_path.BaseName().Extension() == ".framework") {
        framework_path = checked_path;
        break;
      }
    }
  }

  if (!framework_path.empty()) {
    DVLOG(1) << "Framework detected: " << framework_path;
    base::apple::SetOverrideFrameworkBundlePath(framework_path);
  }
}

}  // namespace app_utils
}  // namespace v2
}  // namespace brave_vpn

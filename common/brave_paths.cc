/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_paths.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
#include "base/native_library.h"
#include "chrome/common/chrome_paths.h"
#include "third_party/widevine/cdm/widevine_cdm_common.h"
#endif

namespace brave {

bool PathProvider(int key, base::FilePath* result) {
  base::FilePath cur;

  switch (key) {
    case DIR_TEST_DATA:
      if (!base::PathService::Get(base::DIR_SOURCE_ROOT, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("brave"));
      cur = cur.Append(FILE_PATH_LITERAL("test"));
      cur = cur.Append(FILE_PATH_LITERAL("data"));
      if (!base::PathExists(cur))  // We don't want to create this.
        return false;
      break;

    default:
      return false;
  }

  *result = cur;
  return true;
}

void RegisterPathProvider() {
  base::PathService::RegisterProvider(PathProvider, PATH_START, PATH_END);
}

void OverridePath() {
#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  // Brave downloads cdm lib to user dir when user accepts instead of shippig by
  // default. So, override |DIR_BUNDLED_WIDEVINE_CDM| to new path in user dir.
  base::FilePath widevine_cdm_path;
  if (base::PathService::Get(chrome::DIR_USER_DATA, &widevine_cdm_path)) {
    widevine_cdm_path =
      widevine_cdm_path.AppendASCII(kWidevineCdmBaseDirectory);
    base::PathService::OverrideAndCreateIfNeeded(
        chrome::DIR_BUNDLED_WIDEVINE_CDM, widevine_cdm_path, true, false);
  }
#endif
}

}  // namespace brave

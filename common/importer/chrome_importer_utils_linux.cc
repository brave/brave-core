// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/common/importer/chrome_importer_utils.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"

base::FilePath GetChromeUserDataFolder() {
  base::FilePath result;
  if (!PathService::Get(base::DIR_HOME, &result))
    return base::FilePath();

  result = result.Append(".config");
  result = result.Append("google-chrome");

  return result;
}

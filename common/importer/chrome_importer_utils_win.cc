/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/importer/chrome_importer_utils.h"

#include "base/files/file_util.h"
#include "base/path_service.h"

base::FilePath GetChromeUserDataFolder() {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_LOCAL_APP_DATA, &result))
    return base::FilePath();

  result = result.AppendASCII("Google");
  result = result.AppendASCII("Chrome");
  result = result.AppendASCII("User Data");

  return result;
}

base::FilePath GetCanaryUserDataFolder() {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_LOCAL_APP_DATA, &result))
    return base::FilePath();

  result = result.AppendASCII("Google");
  result = result.AppendASCII("Chrome SxS");
  result = result.AppendASCII("User Data");

  return result;
}

base::FilePath GetChromiumUserDataFolder() {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_LOCAL_APP_DATA, &result))
    return base::FilePath();

  result = result.AppendASCII("Chromium");
  result = result.AppendASCII("User Data");

  return result;
}

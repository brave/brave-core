/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/importer/brave_importer_utils.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string16.h"

base::FilePath GetBraveUserDataFolder() {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_APP_DATA, &result))
    return base::FilePath();

  result = result.AppendASCII("brave");

  return result;
}

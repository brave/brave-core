/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/first_run/first_run_internal.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "chrome/installer/util/master_preferences.h"

namespace first_run {
namespace internal {

bool IsOrganicFirstRun() {
  // We treat all installs as organic.
  return true;
}

base::FilePath MasterPrefsPath() {
  // The standard location of the master prefs is next to the chrome binary.
  base::FilePath master_prefs;
  if (!base::PathService::Get(base::DIR_EXE, &master_prefs))
    return base::FilePath();
  return master_prefs.AppendASCII(installer::kDefaultMasterPrefs);
}

}  // namespace internal
}  // namespace first_run

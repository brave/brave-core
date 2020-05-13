/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/importer/chrome_importer_utils.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"

base::FilePath GetChromeUserDataFolder() {
  return base::FilePath();
}

base::FilePath GetChromiumUserDataFolder() {
  return base::FilePath();
}

base::FilePath GetCanaryUserDataFolder() {
  return base::FilePath();
}

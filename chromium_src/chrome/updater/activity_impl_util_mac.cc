/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/updater/activity_impl_util_posix.h"

#define GetActiveFile GetActiveFile_Unused

#include <chrome/updater/activity_impl_util_mac.cc>

#undef GetActiveFile

namespace updater {

// Chromium < 145.0.7564.0 appends "SoftwareUpdate" to COMPANY_SHORTNAME_STRING.
// This results in "BraveSoftwareSoftwareUpdate" (2x "Software"). Use
// KEYSTONE_NAME instead to get "BraveSoftwareUpdate".
// TODO(https://github.com/brave/brave-browser/issues/51355): Remove this once
// Brave is on Chromium 145.0.7564.0+.
base::FilePath GetActiveFile(const base::FilePath& home_dir,
                             const std::string& id) {
  return home_dir.Append("Library")
      .Append(COMPANY_SHORTNAME_STRING)
      .Append(KEYSTONE_NAME)
      .Append("Actives")
      .Append(id);
}

}  // namespace updater

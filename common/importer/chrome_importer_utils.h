/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_IMPORTER_CHROME_IMPORTER_UTILS_H_
#define BRAVE_COMMON_IMPORTER_CHROME_IMPORTER_UTILS_H_

#include <optional>
#include <string>
#include <vector>

#include "base/values.h"
#include "build/build_config.h"
#include "components/user_data_importer/common/importer_type.h"
#include "extensions/buildflags/buildflags.h"

namespace base {
class FilePath;
}  // namespace base

// Chrome / Chromium paths
// https://chromium.googlesource.com/chromium/src/+/HEAD/docs/user_data_dir.md
base::FilePath GetChromeUserDataFolder();
base::FilePath GetChromeBetaUserDataFolder();
base::FilePath GetChromeDevUserDataFolder();
#if !BUILDFLAG(IS_LINUX)
// Canary is not available on Linux
base::FilePath GetCanaryUserDataFolder();
#endif
base::FilePath GetChromiumUserDataFolder();

base::FilePath GetEdgeUserDataFolder();

base::FilePath GetVivaldiUserDataFolder();
base::FilePath GetOperaUserDataFolder();
base::FilePath GetYandexUserDataFolder();
base::FilePath GetWhaleUserDataFolder();

#if BUILDFLAG(IS_LINUX)
base::FilePath GetOperaSnapUserDataFolder();
#endif
base::Value::List GetChromeSourceProfiles(const base::FilePath& local_state);
bool ChromeImporterCanImport(const base::FilePath& profile,
                             user_data_importer::ImporterType type,
                             uint16_t* services_supported);

#if BUILDFLAG(ENABLE_EXTENSIONS)
std::optional<std::vector<std::string>> GetImportableChromeExtensionsList(
    const base::FilePath& profile_path);
#endif

#endif  // BRAVE_COMMON_IMPORTER_CHROME_IMPORTER_UTILS_H_

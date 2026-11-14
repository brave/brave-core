/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_IMPORTER_CHROME_IMPORTER_UTILS_H_
#define BRAVE_COMMON_IMPORTER_CHROME_IMPORTER_UTILS_H_

#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/user_data_importer/common/importer_type.h"
#include "components/version_info/channel.h"
#include "extensions/buildflags/buildflags.h"

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

// The two Brave product families that install side-by-side with separate user
// data directories: Brave Browser ("Brave-Browser") and Brave Origin
// ("Brave-Origin"). Each ships the same set of release channels.
enum class BraveImporterProduct {
  kBraveBrowser,
  kBraveOrigin,
};

// Returns the leaf user-data directory name for a Brave product/channel combo,
// e.g. "Brave-Browser", "Brave-Browser-Beta", "Brave-Origin-Nightly". The
// STABLE and UNKNOWN channels use the unsuffixed base name.
std::string GetBraveUserDataDirName(BraveImporterProduct product,
                                    version_info::Channel channel);

// Returns the user data folder for the given Brave product and release channel.
base::FilePath GetBraveUserDataFolder(
    BraveImporterProduct product = BraveImporterProduct::kBraveBrowser,
    version_info::Channel channel = version_info::Channel::STABLE);

// A Brave install (product + channel) that can be offered as an import source,
// paired with the display name to show for it.
struct BraveImporterSource {
  BraveImporterProduct product;
  version_info::Channel channel;
  const char* name;
};

// Returns the Brave installs that should be offered as import sources: every
// product/channel combination except the one currently running
// (`current_product` + `current_channel`). A local developer build passes
// UNKNOWN, which matches no shipping channel and therefore excludes nothing.
std::vector<BraveImporterSource> GetBraveImporterSources(
    BraveImporterProduct current_product,
    version_info::Channel current_channel);

#if BUILDFLAG(IS_LINUX)
base::FilePath GetOperaSnapUserDataFolder();
#endif
base::ListValue GetChromeSourceProfiles(const base::FilePath& local_state);
bool ChromeImporterCanImport(const base::FilePath& profile,
                             user_data_importer::ImporterType type,
                             uint16_t* services_supported);

#if BUILDFLAG(ENABLE_EXTENSIONS)
std::optional<std::vector<std::string>> GetImportableChromeExtensionsList(
    const base::FilePath& profile_path);
#endif

#endif  // BRAVE_COMMON_IMPORTER_CHROME_IMPORTER_UTILS_H_

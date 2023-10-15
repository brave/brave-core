/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/importer/importer_list.h"
#include "base/files/file_path.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/values.h"
#include "brave/common/importer/chrome_importer_utils.h"
#include "brave/common/importer/importer_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/common/importer/importer_type.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace {
void AddChromeToProfiles(std::vector<importer::SourceProfile>* profiles,
                         base::Value::List chrome_profiles,
                         const base::FilePath& user_data_folder,
                         const std::string& brand,
                         importer::ImporterType type) {
  for (const auto& value : chrome_profiles) {
    const auto* dict = value.GetIfDict();
    if (!dict)
      continue;
    uint16_t items = importer::NONE;
    auto* profile = dict->FindString("id");
    auto* name = dict->FindString("name");
    DCHECK(profile);
    DCHECK(name);
    base::FilePath path = user_data_folder;
    if (!ChromeImporterCanImport(path.Append(base::FilePath::StringType(
                                     profile->begin(), profile->end())),
                                 &items))
      continue;
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
    // We can import password from Whale only on macOS.
    // Decryption failed on Windows and Linux.
    if (type == importer::TYPE_WHALE && (items & importer::PASSWORDS)) {
      items ^= importer::PASSWORDS;
    }
#endif
    importer::SourceProfile chrome;
    chrome.importer_name = base::UTF8ToUTF16(base::StrCat({brand, " ", *name}));
    chrome.importer_type = type;
    chrome.services_supported = items;
    chrome.source_path = user_data_folder.Append(
        base::FilePath::StringType(profile->begin(), profile->end()));
    profiles->push_back(chrome);
  }
}

void DetectChromeProfiles(std::vector<importer::SourceProfile>* profiles) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  AddChromeToProfiles(
      profiles,
      GetChromeSourceProfiles(GetChromeUserDataFolder().Append(
          base::FilePath::StringType(FILE_PATH_LITERAL("Local State")))),
      GetChromeUserDataFolder(), kGoogleChromeBrowser, importer::TYPE_CHROME);
  AddChromeToProfiles(
      profiles,
      GetChromeSourceProfiles(GetChromeBetaUserDataFolder().Append(
          base::FilePath::StringType(FILE_PATH_LITERAL("Local State")))),
      GetChromeBetaUserDataFolder(), kGoogleChromeBrowserBeta,
      importer::TYPE_CHROME);
  AddChromeToProfiles(
      profiles,
      GetChromeSourceProfiles(GetChromeDevUserDataFolder().Append(
          base::FilePath::StringType(FILE_PATH_LITERAL("Local State")))),
      GetChromeDevUserDataFolder(), kGoogleChromeBrowserDev,
      importer::TYPE_CHROME);
#if !BUILDFLAG(IS_LINUX)
  AddChromeToProfiles(
      profiles,
      GetChromeSourceProfiles(GetCanaryUserDataFolder().Append(
          base::FilePath::StringType(FILE_PATH_LITERAL("Local State")))),
      GetCanaryUserDataFolder(), kGoogleChromeBrowserCanary,
      importer::TYPE_CHROME);
#endif
  AddChromeToProfiles(
      profiles,
      GetChromeSourceProfiles(GetChromiumUserDataFolder().Append(
          base::FilePath::StringType(FILE_PATH_LITERAL("Local State")))),
      GetChromiumUserDataFolder(), kChromiumBrowser, importer::TYPE_CHROME);

  AddChromeToProfiles(
      profiles,
      GetChromeSourceProfiles(GetEdgeUserDataFolder().Append(
          base::FilePath::StringType(FILE_PATH_LITERAL("Local State")))),
      GetEdgeUserDataFolder(), kMicrosoftEdgeBrowser,
      importer::TYPE_EDGE_CHROMIUM);

  AddChromeToProfiles(
      profiles,
      GetChromeSourceProfiles(GetVivaldiUserDataFolder().Append(
          base::FilePath::StringType(FILE_PATH_LITERAL("Local State")))),
      GetVivaldiUserDataFolder(), kVivaldiBrowser, importer::TYPE_VIVALDI);

  AddChromeToProfiles(
      profiles,
      GetChromeSourceProfiles(GetOperaUserDataFolder().Append(
          base::FilePath::StringType(FILE_PATH_LITERAL("Local State")))),
      GetOperaUserDataFolder(), kOperaBrowser, importer::TYPE_OPERA);

  AddChromeToProfiles(
      profiles,
      GetChromeSourceProfiles(GetYandexUserDataFolder().Append(
          base::FilePath::StringType(FILE_PATH_LITERAL("Local State")))),
      GetYandexUserDataFolder(), kYandexBrowser, importer::TYPE_YANDEX);

  AddChromeToProfiles(
      profiles,
      GetChromeSourceProfiles(GetWhaleUserDataFolder().Append(
          base::FilePath::StringType(FILE_PATH_LITERAL("Local State")))),
      GetWhaleUserDataFolder(), kWhaleBrowser, importer::TYPE_WHALE);

#if BUILDFLAG(IS_LINUX)
  // Installed via snap Opera has different profile path.
  AddChromeToProfiles(
      profiles,
      GetChromeSourceProfiles(GetOperaSnapUserDataFolder().Append(
          base::FilePath::StringType(FILE_PATH_LITERAL("Local State")))),
      GetOperaSnapUserDataFolder(), kOperaBrowser, importer::TYPE_OPERA);
#endif
}

}  // namespace

#define IDS_IMPORT_FROM_EDGE_OLD IDS_IMPORT_FROM_EDGE
#undef IDS_IMPORT_FROM_EDGE
#define IDS_IMPORT_FROM_EDGE IDS_BRAVE_IMPORT_FROM_EDGE
#include "src/chrome/browser/importer/importer_list.cc"
#undef IDS_IMPORT_FROM_EDGE
#define IDS_IMPORT_FROM_EDGE IDS_IMPORT_FROM_EDGE_OLD
#undef IDS_IMPORT_FROM_EDGE_OLD

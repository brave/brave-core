/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/values.h"
#include "brave/common/importer/chrome_importer_utils.h"
#include "chrome/browser/importer/importer_list.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace {
void AddChromeToProfiles(std::vector<importer::SourceProfile>* profiles,
                         base::ListValue* chrome_profiles,
                         const base::FilePath& user_data_folder,
                         const std::string& brand) {
  for (const auto& value : chrome_profiles->GetListDeprecated()) {
    const base::DictionaryValue* dict;
    if (!value.GetAsDictionary(&dict))
      continue;
    uint16_t items = importer::NONE;
    std::string profile;
    std::string name;
    dict->GetString("id", &profile);
    dict->GetString("name", &name);
    base::FilePath path = user_data_folder;
    if (!ChromeImporterCanImport(path.Append(
      base::FilePath::StringType(profile.begin(), profile.end())), &items))
      continue;
    importer::SourceProfile chrome;
    std::string importer_name(brand);
    importer_name.append(name);
    chrome.importer_name = base::UTF8ToUTF16(importer_name);
    chrome.importer_type = importer::TYPE_CHROME;
    chrome.services_supported = items;
    chrome.source_path =
      user_data_folder.Append(
        base::FilePath::StringType(profile.begin(), profile.end()));
    profiles->push_back(chrome);
  }
  delete chrome_profiles;
}

void DetectChromeProfiles(std::vector<importer::SourceProfile>* profiles) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  const base::FilePath chrome_user_data_folder = GetChromeUserDataFolder();
  base::ListValue* chrome_profiles =
      GetChromeSourceProfiles(chrome_user_data_folder);
  const std::string brand_chrome("Chrome ");
  AddChromeToProfiles(profiles, chrome_profiles, chrome_user_data_folder,
                      brand_chrome);

#if !defined(OS_LINUX)
  const base::FilePath canary_user_data_folder = GetCanaryUserDataFolder();
  base::ListValue* canary_profiles =
      GetChromeSourceProfiles(canary_user_data_folder);
  const std::string brandCanary("Chrome Canary ");
  AddChromeToProfiles(profiles, canary_profiles, canary_user_data_folder,
                      brandCanary);
#endif

  const base::FilePath chromium_user_data_folder = GetChromiumUserDataFolder();
  base::ListValue* chromium_profiles =
      GetChromeSourceProfiles(chromium_user_data_folder);
  const std::string brandChromium("Chromium ");
  AddChromeToProfiles(profiles, chromium_profiles, chromium_user_data_folder,
                      brandChromium);
}

}  // namespace

#include "src/chrome/browser/importer/importer_list.cc"

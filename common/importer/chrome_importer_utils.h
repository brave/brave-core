// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMMON_IMPORTER_CHROME_IMPORTER_UTILS_H_
#define BRAVE_COMMON_IMPORTER_CHROME_IMPORTER_UTILS_H_

#include <stdint.h>

#include <vector>

namespace base {
class DictionaryValue;
class FilePath;
class ListValue;
}

base::FilePath GetChromeUserDataFolder();

base::ListValue* GetChromeSourceProfiles(
  const base::FilePath& user_data_folder);

bool ChromeImporterCanImport(const base::FilePath& profile,
                             uint16_t* services_supported);

#endif  // BRAVE_COMMON_IMPORTER_CHROME_IMPORTER_UTILS_H_

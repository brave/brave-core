/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_IMPORTER_BRAVE_IMPORTER_UTILS_H_
#define BRAVE_COMMON_IMPORTER_BRAVE_IMPORTER_UTILS_H_

#include <stdint.h>

#include <vector>

namespace base {
class DictionaryValue;
class FilePath;
class ListValue;
}

base::FilePath GetBraveUserDataFolder();

base::ListValue* GetBraveSourceProfiles(
    const base::FilePath& user_data_folder);

bool BraveImporterCanImport(const base::FilePath& profile,
			    uint16_t* services_supported);

#endif  // BRAVE_COMMON_IMPORTER_BRAVE_IMPORTER_UTILS_H_

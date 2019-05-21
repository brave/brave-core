/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_DAT_FILE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_DAT_FILE_UTIL_H_

#include <string>
#include <vector>

namespace base {
class FilePath;
}

namespace brave_component_updater {

using DATFileDataBuffer = std::vector<unsigned char>;

void GetDATFileData(const base::FilePath& file_path,
                    DATFileDataBuffer* buffer);
void GetDATFileAsString(const base::FilePath& file_path,
                        std::string* contents);

}  // namespace brave_component_updater

#endif  // BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_DAT_FILE_UTIL_H_

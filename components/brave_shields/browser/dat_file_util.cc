/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/dat_file_util.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"

namespace brave_shields {

bool GetDATFileData(const std::string& fileName,
    std::vector<unsigned char>& buffer) {
  base::FilePath app_data_path;
  PathService::Get(chrome::DIR_USER_DATA, &app_data_path);

  base::FilePath dataFilePath = app_data_path.Append(fileName);
  int64_t size = 0;
  if (!base::PathExists(dataFilePath)
      || !base::GetFileSize(dataFilePath, &size)
      || 0 == size) {
    LOG(ERROR) << "GetDATFileData: "
      << "the dat file is not found or corrupted "
      << dataFilePath;
    return false;
  }
  buffer.resize(size);
  if (size != base::ReadFile(dataFilePath, (char*)&buffer.front(), size)) {
    LOG(ERROR) << "GetDATFileData: cannot "
      << "read dat file " << fileName;
     return false;
  }

  LOG(ERROR) << "Initialized brave shields service correctly";
  return true;
}

}  // namespace brave_shields

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/dat_file_util.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"

namespace brave_shields {

bool GetDATFileData(const base::FilePath& dir,
    const std::string& file_name,
    std::vector<unsigned char>& buffer) {
  base::FilePath dat_file_path = dir.AppendASCII(file_name);
  int64_t size = 0;
  if (!base::PathExists(dat_file_path)
      || !base::GetFileSize(dat_file_path, &size)
      || 0 == size) {
    LOG(ERROR) << "GetDATFileData: "
      << "the dat file is not found or corrupted "
      << dat_file_path;
    return false;
  }
  buffer.resize(size);
  if (size != base::ReadFile(dat_file_path, (char*)&buffer.front(), size)) {
    LOG(ERROR) << "GetDATFileData: cannot "
      << "read dat file " << file_name;
     return false;
  }

  LOG(ERROR) << "Initialized brave shields service correctly";
  return true;
}

}  // namespace brave_shields

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/dat_file_util.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"

namespace brave_shields {

bool GetDATFileData(const base::FilePath& file_path,
    std::vector<unsigned char>& buffer) {
  int64_t size = 0;
  if (!base::PathExists(file_path)
      || !base::GetFileSize(file_path, &size)
      || 0 == size) {
    LOG(ERROR) << "GetDATFileData: "
      << "the dat file is not found or corrupted "
      << file_path;
    return false;
  }
  buffer.resize(size);
  if (size != base::ReadFile(file_path, (char*)&buffer.front(), size)) {
    LOG(ERROR) << "GetDATFileData: cannot "
      << "read dat file " << file_path;
     return false;
  }

  return true;
}

}  // namespace brave_shields

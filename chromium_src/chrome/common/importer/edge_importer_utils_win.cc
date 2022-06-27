/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <Shlobj.h>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/win/windows_version.h"

namespace importer {

base::FilePath GetEdgeDataFilePath() {
  wchar_t buffer[MAX_PATH];
  if (::SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT,
                        buffer) != S_OK)
    return base::FilePath();

  base::FilePath base_path(buffer);
  return base_path.Append(L"Microsoft\\Edge\\User Data\\Default");
}

bool EdgeImporterCanImport() {
  base::File::Info file_info;
  if (base::win::GetVersion() < base::win::Version::WIN10)
    return false;
  return base::GetFileInfo(GetEdgeDataFilePath(), &file_info) &&
         file_info.is_directory;
}

}  // namespace importer

#define EdgeImporterCanImport EdgeImporterCanImport_ChromiumImpl
#define GetEdgeDataFilePath GetEdgeDataFilePath_ChromiumImpl
#include "src/chrome/common/importer/edge_importer_utils_win.cc"
#undef GetEdgeDataFilePath
#undef EdgeImporterCanImport

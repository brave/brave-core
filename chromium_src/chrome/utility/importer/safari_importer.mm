/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <Cocoa/Cocoa.h>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "chrome/utility/importer/safari_importer.h"

#define fileSystemRepresentation fileSystemRepresentation];                   \
  int64_t file_size = 0;                                                      \
  if (base::GetFileSize(base::FilePath(db_path), &file_size) && !file_size) { \
    return false;                                                             \
  }                                                                           \
  VLOG(1) << [favicons_db_path fileSystemRepresentation
#include "src/chrome/utility/importer/safari_importer.mm"
#undef fileSystemRepresentation

/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/importer/scoped_copy_file.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"

ScopedCopyFile::ScopedCopyFile(const base::FilePath& original_file_path) {
  DCHECK(base::PathExists(original_file_path));
  if (base::CreateTemporaryFile(&copied_file_path_))
    copy_success_ = base::CopyFile(original_file_path, copied_file_path_);
}

ScopedCopyFile::~ScopedCopyFile() {
  if (base::PathExists(copied_file_path_))
    base::DeleteFile(copied_file_path_);
}

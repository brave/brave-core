/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_IMPORTER_SCOPED_COPY_FILE_H_
#define BRAVE_COMMON_IMPORTER_SCOPED_COPY_FILE_H_

#include "base/files/file_path.h"

class ScopedCopyFile {
 public:
  explicit ScopedCopyFile(const base::FilePath& original_file_path);
  ~ScopedCopyFile();

  ScopedCopyFile(const ScopedCopyFile&) = delete;
  ScopedCopyFile& operator=(const ScopedCopyFile&) = delete;

  bool copy_success() const { return copy_success_; }
  base::FilePath copied_file_path() const { return copied_file_path_; }

 private:
  bool copy_success_ = false;
  base::FilePath copied_file_path_;
};

#endif  // BRAVE_COMMON_IMPORTER_SCOPED_COPY_FILE_H_

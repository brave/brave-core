/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WIDEVINE_BRAVE_WIDEVINE_BUNDLE_UNZIPPER_H_
#define BRAVE_BROWSER_WIDEVINE_BRAVE_WIDEVINE_BUNDLE_UNZIPPER_H_

#include <string>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"

namespace base {
class SequencedTaskRunner;
}

namespace service_manager {
class Connector;
}

class BraveWidevineBundleUnzipper
    : public base::RefCountedThreadSafe<BraveWidevineBundleUnzipper> {
 public:
  using DoneCallback = base::OnceCallback<void(const std::string& error)>;

  static scoped_refptr<BraveWidevineBundleUnzipper> Create(
      scoped_refptr<base::SequencedTaskRunner> file_task_runner,
      DoneCallback done_callback);

  // Unzips widevine cdm library of |zipped_bundle_file| into |unzip_dir|.
  // This will remove passed |zipped_bundle_file| if |delete_file| is true.
  void LoadFromZipFileInDir(const base::FilePath& zipped_bundle_file,
                            const base::FilePath& unzip_dir,
                            bool delete_file);

 private:
  friend class base::RefCountedThreadSafe<BraveWidevineBundleUnzipper>;

  BraveWidevineBundleUnzipper(
      scoped_refptr<base::SequencedTaskRunner> file_task_runner,
      DoneCallback done_callback);
  ~BraveWidevineBundleUnzipper();

  void OnGetTempDirForUnzip(base::Optional<base::FilePath> temp_unzip_dir);
  void OnUnzippedInTempDir(bool status);

  std::string MoveUnzippedLibFromTempToTargetDir();

  // Called all steps are fininshed.
  // |error| is not empty if there is an error during the unzip.
  void UnzipDone(const std::string& error);

  bool delete_zip_file_ = false;
  base::FilePath zipped_bundle_file_;
  base::FilePath target_unzip_dir_;

  // Temp dir is used because unzipper failed when target file is
  // already existed. This can be happened when widevine lib is updated.
  // So, lib is extracted to temp dir then copied to |target_unzip_dir_|.
  base::FilePath temp_unzip_dir_;

  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  DoneCallback done_callback_;

  DISALLOW_COPY_AND_ASSIGN(BraveWidevineBundleUnzipper);
};

#endif  // BRAVE_BROWSER_WIDEVINE_BRAVE_WIDEVINE_BUNDLE_UNZIPPER_H_

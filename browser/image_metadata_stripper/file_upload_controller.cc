/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/browser/image_metadata_stripper/file_upload_controller.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "chrome/common/chrome_paths.h"
#include "components/tabs/public/tab_interface.h"

namespace image_metadata_stripper {
base::FilePath GetStripperRootDirectory() {
  return base::PathService::CheckedGet(chrome::DIR_USER_DATA)
      .Append(kStripperRootDirName);
}

class FileUploadController::Delegate {
 public:
  Delegate() = default;
  Delegate(const Delegate&) = delete;
  Delegate& operator=(const Delegate&) = delete;

  ~Delegate() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    if (dir_.IsValid() && !dir_.Delete()) {
      LOG(ERROR) << "Failed to delete image strip temp directory.";
    }
  }

  std::vector<std::optional<base::FilePath>> StripAll(
      std::vector<base::FilePath> srcs,
      StrippingClient client) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    std::vector<std::optional<base::FilePath>> copies;
    copies.reserve(srcs.size());
    for (const base::FilePath& src : srcs) {
      copies.push_back(Strip(src, client));
    }
    return copies;
  }

 private:
  std::optional<base::FilePath> Strip(const base::FilePath& src,
                                      StrippingClient client) {
    const base::FilePath basename = src.BaseName();
    // "" or "../" cannot name the copy, and the latter could escape the
    // isolation the temporary root directory provides.
    if (basename.empty() || basename.ReferencesParent()) {
      return std::nullopt;
    }

    if (!IsSupportedImagePath(src) || !ContainsMetadataToStrip(src)) {
      return std::nullopt;
    }

    if (!InitDir()) {
      return std::nullopt;
    }

    // Give the copy a subdirectory of its own so that it can keep the basename
    // of |src| without colliding with the other copies.
    base::ScopedTempDir sub_dir;
    if (!sub_dir.Set(dir_.GetPath().AppendASCII(
            base::NumberToString(next_sub_dir_index_++)))) {
      LOG(ERROR) << "Image strip skipped; temp subdir could not be created: "
                 << src;
      return std::nullopt;
    }

    const base::FilePath copy = sub_dir.GetPath().Append(basename);
    if (!base::CopyFile(src, copy)) {
      DVLOG(1) << "Image strip skipped; failed to copy the image file to a "
                  "temporary file.";
      return std::nullopt;
    }

    if (!RemoveIptcMetadata(client, copy)) {
      DVLOG(1) << "No stripping occured; keeping original: " << src;
      // ScopedTempDir deletes the subdir, so the failed copy leaves nothing
      // behind.
      return std::nullopt;
    }

    // Keep the copy under the root until the root is deleted.
    sub_dir.Take();
    return copy;
  }

  bool InitDir() {
    if (dir_.IsValid()) {
      return true;
    }

    // Create the component root directory
    const base::FilePath base_dir = GetStripperRootDirectory();
    if (!base::PathExists(base_dir)) {
      base::CreateDirectory(base_dir);
    }

    if (!dir_.CreateUniqueTempDirUnderPath(base_dir, kStripTempDirPrefix)) {
      LOG(ERROR) << "Image strip skipped; temp directory could not be created.";
      return false;
    }

    return true;
  }

  SEQUENCE_CHECKER(sequence_checker_);

  // The tab level temporary directory we create inside the
  // |kStripperRootDirName|.
  base::ScopedTempDir dir_;
  // This points to the name of the subdirectory inside |kStripperRootDirName| >
  // |dir_| where the next stripped copy will go.
  size_t next_sub_dir_index_ = 0;
};

DEFINE_USER_DATA(FileUploadController);

FileUploadController::FileUploadController(tabs::TabInterface& tab)
    : tabs::ContentsObservingTabFeature(tab),
      task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      scoped_unowned_user_data_(tab.GetUnownedUserDataHost(), *this) {}

FileUploadController::~FileUploadController() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

// static
FileUploadController* FileUploadController::FromWebContents(
    content::WebContents* web_contents) {
  return web_contents
             ? From(tabs::TabInterface::MaybeGetFromContents(web_contents))
             : nullptr;
}

// static
void FileUploadController::CleanupDir() {
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(
          [](const base::FilePath& path) {
            if (!base::DeletePathRecursively(path)) {
              LOG(ERROR) << "Failed to delete image strip temp directory: "
                         << path;
            }
          },
          GetStripperRootDirectory()));
}

void FileUploadController::Strip(std::vector<base::FilePath> srcs,
                                 StrippingClient client,
                                 StripCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!delegate_) {
    delegate_.emplace(task_runner_);
  }
  delegate_.AsyncCall(&Delegate::StripAll)
      .WithArgs(std::move(srcs), client)
      .Then(std::move(callback));
}

// static
FileUploadController* FileUploadController::From(tabs::TabInterface* tab) {
  return tab ? Get(tab->GetUnownedUserDataHost()) : nullptr;
}

// static
std::unique_ptr<FileUploadController> FileUploadController::MaybeCreate(
    tabs::TabInterface& tab) {
  // No controller must have been created at this point.
  auto* controller = Get(tab.GetUnownedUserDataHost());
  CHECK(!controller);

  return base::WrapUnique(new FileUploadController(tab));
}

void FileUploadController::OnDiscardContents(
    tabs::TabInterface* tab,
    content::WebContents* old_contents,
    content::WebContents* new_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  delegate_.Reset();
  tabs::ContentsObservingTabFeature::OnDiscardContents(tab, old_contents,
                                                       new_contents);
}

}  // namespace image_metadata_stripper

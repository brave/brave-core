/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/drag_drop/brave_drag_drop_image_metadata_stripper.h"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/drop_data.h"
#include "ui/base/clipboard/file_info.h"

namespace brave {

namespace {

using DropCompletionCallback =
    content::WebContentsViewDelegate::DropCompletionCallback;

bool HasSupportedImage(const content::DropData& drop_data) {
  return std::ranges::any_of(drop_data.filenames, [](const ui::FileInfo& file) {
    return image_metadata_stripper::IsSupportedImagePath(file.path);
  });
}

void DeleteTempRootDirsOnBlockingThread(std::vector<base::FilePath> dirs) {
  for (const base::FilePath& dir : dirs) {
    image_metadata_stripper::DeleteStrippedImageCopies(dir);
  }
}

void DeleteTempRootDirs(std::vector<base::FilePath> dirs) {
  if (dirs.empty()) {
    return;
  }

  // File I/O needs to run on blocking thread. We use BLOCK_SHUTDOWN to ensure
  // the temporary files are deleted promptly before shutdown can happen. Having
  // a clean-up logic around startup may work to relax this.
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::BindOnce(&DeleteTempRootDirsOnBlockingThread, std::move(dirs)));
}

// Owns the temporary directories holding the stripped copies handed to a drop
// target. The `File` the page receives is backed by the copy on disk, so the
// copy has to outlive the drop itself; the WebContents going away is the point
// at which the page can no longer reach it. This is the same lifetime
// FileSelectHelper gives the copies made for an upload.
class DropStripTempDirs
    : public content::WebContentsUserData<DropStripTempDirs> {
 public:
  ~DropStripTempDirs() override {
    DeleteTempRootDirs(std::move(temp_root_dirs_));
    temp_root_dirs_.clear();
  }

  void Add(base::FilePath temp_root_dir) {
    temp_root_dirs_.push_back(std::move(temp_root_dir));
  }

 private:
  friend content::WebContentsUserData<DropStripTempDirs>;

  explicit DropStripTempDirs(content::WebContents* web_contents)
      : content::WebContentsUserData<DropStripTempDirs>(*web_contents) {}

  std::vector<base::FilePath> temp_root_dirs_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

// Needed for maintenace purposes by the `DropStripTempDirs` class.
// This will ensure when the WebContents is destroyed, the ~DropStripTempDirs is
// called.
[[__maybe_unused__]] WEB_CONTENTS_USER_DATA_KEY_IMPL(DropStripTempDirs);

struct StripResult {
  content::DropData drop_data;
  // Temporary root directory holding the stripped copies. Empty when nothing
  // was stripped.
  base::FilePath temp_root_dir;
};

StripResult StripDropDataOnBlockingThread(content::DropData drop_data) {
  image_metadata_stripper::StrippedImageCopier copier(
      image_metadata_stripper::StrippingClient::kDragDrop);

  for (ui::FileInfo& file : drop_data.filenames) {
    if (auto copy = copier.MaybeCreateStrippedCopy(file.path)) {
      file.path = *std::move(copy);
    }
  }

  StripResult result;
  if (copier.has_copies()) {
    // Take() so the copies survive until the tab closes.
    result.temp_root_dir = copier.TakeTempRootDir();
  }
  result.drop_data = std::move(drop_data);
  return result;
}

// The callback which gets fired after all the stripping was completed.
void OnStripComplete(base::WeakPtr<content::WebContents> web_contents,
                     DropCompletionCallback callback,
                     StripResult result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!result.temp_root_dir.empty()) {
    if (web_contents) {
      DropStripTempDirs::CreateForWebContents(web_contents.get());
      DropStripTempDirs::FromWebContents(web_contents.get())
          ->Add(std::move(result.temp_root_dir));
    } else {
      // The tab went away while stripping; nothing is left to hand the copies
      // to, and the view will discard the drop.
      DeleteTempRootDirs({std::move(result.temp_root_dir)});
    }
  }

  std::move(callback).Run(std::move(result.drop_data));
}

void MaybeStripDropData(base::WeakPtr<content::WebContents> web_contents,
                        DropCompletionCallback callback,
                        std::optional<content::DropData> drop_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Nothing to do when the drop was blocked upstream, when the tab it targets
  // is gone, or when no dropped file could carry the metadata we strip.
  if (!drop_data || !web_contents || !HasSupportedImage(*drop_data)) {
    std::move(callback).Run(std::move(drop_data));
    return;
  }

  // Stripping begins.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&StripDropDataOnBlockingThread, std::move(*drop_data)),
      base::BindOnce(&OnStripComplete, web_contents, std::move(callback)));
}

}  // namespace

DropCompletionCallback MaybeStripImageMetadataForDrop(
    content::WebContents* web_contents,
    DropCompletionCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK(callback);

  if (!base::FeatureList::IsEnabled(
          image_metadata_stripper::features::kStripImageMetadataV1)) {
    return callback;
  }

  return base::BindOnce(&MaybeStripDropData, web_contents->GetWeakPtr(),
                        std::move(callback));
}

}  // namespace brave

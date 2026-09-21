/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/file_select/brave_file_select_image_metadata_stripper.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/containers/extend.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper_utils.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom.h"

namespace brave {

namespace {

// IN-TEST
base::OnceCallback<void(std::vector<base::FilePath>)>*
    g_on_strip_completed_callback_for_testing_ = nullptr;

struct StripResult {
  // The final list of selected files where the original image file(s)
  // containing flagged metadata gets replaced with a stripped-out
  // temporary image file w/o the flagged metadata.
  std::vector<blink::mojom::FileChooserFileInfoPtr> selected_files;
  // Temporary files which are created during the stripping process. These files
  // are passed down to the upstream for deletion at `OnStripComplete`.
  std::vector<base::FilePath> temp_files;

  StripResult() = default;
  ~StripResult() = default;

  StripResult(const StripResult&) = delete;
  StripResult& operator=(const StripResult&) = delete;

  StripResult(StripResult&&) = default;
  StripResult& operator=(StripResult&&) = default;
};

StripResult StripListOnBlockingThread(
    std::vector<blink::mojom::FileChooserFileInfoPtr> selected_files) {
  image_metadata_stripper::StrippedImageCopier copier(
      image_metadata_stripper::StrippingClient::kFileSelect);

  for (auto& info : selected_files) {
    // File issues. Skip.
    if (!info || !info->is_native_file()) {
      continue;
    }
    auto& native = info->get_native_file();

    auto copy = copier.MaybeCreateStrippedCopy(native->file_path);
    if (!copy) {
      continue;
    }

    // We are swapping out the path of the original file with the copy which has
    // been stripped of metadata. On macOS file controls show the name the user
    // picked, and the copy carries the same basename.
    if (native->display_name.empty()) {
      native->display_name = native->file_path.BaseName().AsUTF16Unsafe();
    }
    native->file_path = copy.value();
  }

  StripResult result;
  if (copier.has_copies()) {
    // Queue the temporary root for FileSelectHelper cleanup. Take() so the
    // copies survive until the tab closes.
    result.temp_files.push_back(copier.TakeTempRootDir());
  }
  result.selected_files = std::move(selected_files);
  return result;
}

// The callback which gets fired after all the stripping was completed.
void OnStripComplete(
    std::vector<base::FilePath>& temporary_files,
    base::OnceCallback<void(std::vector<blink::mojom::FileChooserFileInfoPtr>)>
        notify,
    StripResult result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Test guarded code to get the list of temporary files we created from our
  // component.
  if (g_on_strip_completed_callback_for_testing_) {
    CHECK_IS_TEST();
    CHECK(!g_on_strip_completed_callback_for_testing_->is_null());
    std::move(*g_on_strip_completed_callback_for_testing_)
        .Run(result.temp_files);
    g_on_strip_completed_callback_for_testing_ = nullptr;
  }

  // Update the |temporary_files| list to mark the deletion of our newly created
  // temp files.
  base::Extend(temporary_files, std::move(result.temp_files));
  std::move(notify).Run(std::move(result.selected_files));
}

}  // namespace

bool MaybeStripImageMetadataForUpload(
    bool& already_processed,
    std::vector<base::FilePath>& temporary_files,
    std::vector<blink::mojom::FileChooserFileInfoPtr>& list,
    base::OnceCallback<void(std::vector<blink::mojom::FileChooserFileInfoPtr>)>
        notify) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!base::FeatureList::IsEnabled(
          image_metadata_stripper::features::kStripImageMetadataV1)) {
    return false;
  }

  // A flag to ensure we don't have infinite loops between the caller and
  // callee once the metadata removal task get posted and the instruction
  // pointer returns back to the caller.
  if (already_processed) {
    return false;
  }

  already_processed = true;

  // Stripping begins.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&StripListOnBlockingThread, std::move(list)),
      base::BindOnce(&OnStripComplete, std::ref(temporary_files),
                     std::move(notify)));
  return true;
}

void MaybeDeleteImageMetadataStripperTemporaryDir(
    std::vector<base::FilePath>& paths) {
  // This only makes sense when the feature is enabled.
  if (!base::FeatureList::IsEnabled(
          image_metadata_stripper::features::kStripImageMetadataV1)) {
    return;
  }

  const auto temp_root_dir =
      std::ranges::find_if(paths, [](const base::FilePath& path) {
        return !path.empty() && !path.ReferencesParent() &&
               path.BaseName().value().find(
                   image_metadata_stripper::kStripTempDirPrefix) !=
                   base::FilePath::StringType::npos &&
               base::DirectoryExists(path);
      });

  if (temp_root_dir == paths.end()) {
    return;
  }

  image_metadata_stripper::DeleteStrippedImageCopies(*temp_root_dir);

  // Remove from the list.
  paths.erase(temp_root_dir);
}

void SetStripCompletedCallbackForTesting(  // IN-TEST
    base::OnceCallback<void(std::vector<base::FilePath>)>* callback) {
  g_on_strip_completed_callback_for_testing_ = callback;
}

}  // namespace brave

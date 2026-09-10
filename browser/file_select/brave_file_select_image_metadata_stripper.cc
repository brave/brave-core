/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/file_select/brave_file_select_image_metadata_stripper.h"

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/containers/extend.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
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

// TODO(https://github.com/brave/brave-browser/issues/5238): PNG formats needs
// more investigation whether FBMD is present or not. So, tackling only jpeg.
bool IsStrippableImagePath(const base::FilePath& path) {
  return path.MatchesExtension(FILE_PATH_LITERAL(".jpg")) ||
         path.MatchesExtension(FILE_PATH_LITERAL(".jpeg"));
}

// Returns true if any of the items in the |selected_files| could be a candidate
// for stripping metadata.
bool HasStrippableImage(
    const std::vector<blink::mojom::FileChooserFileInfoPtr>& selected_files) {
  return std::ranges::any_of(
      selected_files, [](const blink::mojom::FileChooserFileInfoPtr& info) {
        return info && info->is_native_file() &&
               IsStrippableImagePath(info->get_native_file()->file_path);
      });
}

// Algorithm:
// 1) Create one unique temporary root directory for this selection.
// 2) Iterate over each item in the |selected_files|.
// 3) If the "ith" item is not strippable, continue with 2.
// 4) If the "ith" is strippable then:
//    4.a) Copy it into a unique `ScopedTempDir` subdir of the root, and use the
//    filename of the original file. On macOS file controls show the name the
//    user picked. 4.b) Try and strip the metadata from that copy.
//         4.b.1) If failed: ScopedTempDir automatically deletes the subdir.
//         4.b.2) Otherwise, Take() the subdir and mark the copy for upload.
// 5) If nothing was stripped, ScopedTempDir deletes the temporary root
// directory created in step 1.
// 6) Else Take() the path and queue it for deletion via |temporary_files|.
StripResult StripListOnBlockingThread(
    std::vector<blink::mojom::FileChooserFileInfoPtr> selected_files) {
  StripResult result;

  // 1) Create one unique temporary root directory for this selection.
  base::ScopedTempDir temp_root;
  if (!temp_root.CreateUniqueTempDir(kUploadStripTempDirPrefix)) {
    LOG(ERROR) << "Upload strip skipped; temp directory could not be created.";
    result.selected_files = std::move(selected_files);
    return result;
  }
  const base::FilePath& temp_root_dir = temp_root.GetPath();

  // This will be used to create the sub directory inside the |temp_root_dir|.
  size_t temp_sub_dir_index = 0;

  bool stripped_any = false;
  // 2. Iterate over each item in the |selected_files|.
  for (auto& info : selected_files) {
    // File issues. Skip.
    if (!info || !info->is_native_file()) {
      continue;
    }
    auto& native = info->get_native_file();
    const base::FilePath& src = native->file_path;

    const base::FilePath basename = src.BaseName();
    // File issues. Skip. "" or "../" as they can't be used to create a
    // corresponding temporary file with the same base name. The later could
    // escpae the |temp_root_dir| isolation.
    if (basename.empty() || basename.ReferencesParent()) {
      continue;
    }

    // 3. If the "ith" item is not strippable, continue with 2.
    if (!IsStrippableImagePath(src)) {
      continue;
    }

    // 4.a) Copy it into a unique subdir of the root, ...
    base::ScopedTempDir sub_dir;
    if (!sub_dir.Set(temp_root_dir.AppendASCII(
            base::NumberToString(temp_sub_dir_index++)))) {
      LOG(ERROR) << "Upload strip skipped; temp subdir could not be created: "
                 << src;
      continue;
    }

    // 4.a) ... and use the filename of the original file.
    const base::FilePath temp_stripped_file =
        sub_dir.GetPath().Append(basename);
    if (!base::CopyFile(src, temp_stripped_file)) {
      DVLOG(1) << "Upload strip skipped; Failed to copy the image file to a "
                  "temporary file.";
      continue;
    }

    // 4.b) Try and strip the metadata from that copy.
    if (!RemoveIptcMetadata(
            image_metadata_stripper::StrippingClient::kFileSelect,
            temp_stripped_file)) {
      DVLOG(1) << "No stripping occured; keeping original: " << src;
      // 4.b.1) If failed: ScopedTempDir deletes the subdir.
      continue;
    }

    // 4.b.2) Otherwise, mark the copy for upload.
    // We are swapping out the path of the original file with
    // |temp_stripped_file| which has been stripped of metadata.
    native->file_path = temp_stripped_file;
    if (native->display_name.empty()) {
      native->display_name = basename.AsUTF16Unsafe();
    }
    // Keep the stripped copy under the parent until FileSelectHelper cleanup.
    sub_dir.Take();
    stripped_any = true;
  }

  if (stripped_any) {
    // Queue the temporary root for FileSelectHelper cleanup. Take() so the
    // copies survive until the tab closes.
    result.temp_files.push_back(temp_root.Take());
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

  if (!HasStrippableImage(list)) {
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
  const auto temp_root_dir =
      std::ranges::find_if(paths, [](const base::FilePath& path) {
        return !path.empty() && !path.ReferencesParent() &&
               path.BaseName().value().find(kUploadStripTempDirPrefix) !=
                   base::FilePath::StringType::npos &&
               base::DirectoryExists(path);
      });

  if (temp_root_dir == paths.end()) {
    return;
  }

  if (!base::DeletePathRecursively(*temp_root_dir)) {
    LOG(ERROR) << "Failed to delete the temporary directory for image metadata "
                  "stripper. dir:"
               << *temp_root_dir;
  }

  // Remove from the list.
  paths.erase(temp_root_dir);
}

void SetStripCompletedCallbackForTesting(  // IN-TEST
    base::OnceCallback<void(std::vector<base::FilePath>)>* callback) {
  g_on_strip_completed_callback_for_testing_ = callback;
}

}  // namespace brave

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/file_select/brave_file_select_image_metadata_stripper.h"

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
#include "content/public/browser/browser_thread.h"

namespace brave {

namespace {

struct StripResult {
  std::vector<blink::mojom::FileChooserFileInfoPtr> list;
  std::vector<base::FilePath> temp_files;
};

// TODO(https://github.com/brave/brave-browser/issues/5238): PNG formats needs
// more investigation whether FBMD is present or not. So, tackling only jpeg.
bool IsStrippableImagePath(const base::FilePath& path) {
  return path.MatchesExtension(FILE_PATH_LITERAL(".jpg")) ||
         path.MatchesExtension(FILE_PATH_LITERAL(".jpeg"));
}

// Returns true if any of the items in the |list| could be a candidate for
// stripping metadata.
bool HasStrippableImage(
    const std::vector<blink::mojom::FileChooserFileInfoPtr>& list) {
  return std::ranges::any_of(
      list, [](const blink::mojom::FileChooserFileInfoPtr& info) {
        return info && info->is_native_file() &&
               IsStrippableImagePath(info->get_native_file()->file_path);
      });
}

// Algorithm:
// 1) Iterate over each item in the |list|.
// 2) If the "ith" item is not strippable, continue with 1.
// 3) If the "ith" is stripppable then:
//    3.a) Copy the contents of "ith" item into a temporary file.
//    3.b) Try and strip the metadata from the temporary file.
//         3.b.1) If failed: Delete the temporary file and go to Step 1.
//         3.b.2) Otherwise, mark the temporay file for upload and then later
//         for deletion.
StripResult StripListOnBlockingThread(
    std::vector<blink::mojom::FileChooserFileInfoPtr> list) {
  StripResult result;
  for (auto& info : list) {
    if (!info || !info->is_native_file()) {
      continue;
    }
    const base::FilePath src = info->get_native_file()->file_path;
    if (!IsStrippableImagePath(src)) {
      continue;
    }

    base::FilePath temp;
    if (!base::CreateTemporaryFile(&temp)) {
      DVLOG(1) << "Upload strip skipped; no temp file for: " << src;
      continue;
    }

    if (!base::CopyFile(src, temp)) {
      DVLOG(1) << "Upload strip skipped; Failed to copy the image file to a "
                  "temporary file.";
    }

    // We try and remove the iptc metadata from the file.
    const bool success = RemoveIptcMetadata(
        image_metadata_stripper::StrippingClient::kFileSelect, temp);
    if (!success) {
      DVLOG(1) << "No stripping occured; keeping original: " << src;
      // The gaurd helps to schedule the delete to upstream's delete lifecycle
      // if ever our own attempt to delete the temporary file failed.
      if (!base::DeleteFile(temp)) {
        result.temp_files.push_back(std::move(temp));
      }
      continue;
    }

    // Re-write the file path of the original upload file, with our temporary's
    // file path. This keeps the overall |list| untouched which is then moved
    // directly to the result.
    // TODO(https://github.com/brave/brave-browser/issues/5238): On macOS the
    // file control shows the temp basename (e.g.
    // .com.brave.Browser.channelNameHere.XXXXXX). From LLM it seems the reason
    // to be the following: LayoutThemeMac::DisplayNameForFile uses
    // NSFileManager displayNameAtPath of the backing path so even setting
    // display_name / File.name is not enough. See
    // https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/
    // renderer/core/layout/layout_theme_mac.mm;l=60 for details.
    // Need to figure out how to handle this issue.
    info->get_native_file()->file_path = temp;

    // Mark the temporary file for deletion later via the upstream's
    // DeleteTemporaryFiles method.
    result.temp_files.push_back(std::move(temp));
  }
  result.list = std::move(list);
  return result;
}

// The callback which gets fired after all the stripping was completed.
void OnStripComplete(
    std::vector<base::FilePath>& temporary_files,
    base::OnceCallback<void(std::vector<blink::mojom::FileChooserFileInfoPtr>)>
        notify,
    StripResult result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // Update the |temporary_files| list to mark the deletion of our newly created
  // tmp files.
  for (auto& temp : result.temp_files) {
    temporary_files.push_back(std::move(temp));
  }
  std::move(notify).Run(std::move(result.list));
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

}  // namespace brave

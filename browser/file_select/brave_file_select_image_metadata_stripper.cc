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

bool IsStrippableImagePath(const base::FilePath& path) {
  return path.MatchesExtension(FILE_PATH_LITERAL(".jpg")) ||
         path.MatchesExtension(FILE_PATH_LITERAL(".jpeg"));
}

bool HasStrippableImage(
    const std::vector<blink::mojom::FileChooserFileInfoPtr>& list) {
  return std::ranges::any_of(
      list, [](const blink::mojom::FileChooserFileInfoPtr& info) {
        return info && info->is_native_file() &&
               IsStrippableImagePath(info->get_native_file()->file_path);
      });
}

struct StripResult {
  std::vector<blink::mojom::FileChooserFileInfoPtr> list;
  std::vector<base::FilePath> temp_files;
};

// Copies each strippable image to a temporary file,
// scrubs the copy, and rewrites the entry to point at it. On any failure the
// original path is left in place so a bad copy is never uploaded.
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

    const auto resultCode = RemoveIptcMetadata(
        image_metadata_stripper::StrippingClient::kFileSelect, temp);

    if (resultCode != image_metadata_stripper::StrippingResultCode::kStripped) {
      DVLOG(1) << "Upload strip failed; keeping original: " << src;
      base::DeleteFile(temp);
      continue;
    }

    info->get_native_file()->file_path = temp;
    result.temp_files.push_back(std::move(temp));
  }
  result.list = std::move(list);
  return result;
}

void OnStripComplete(
    std::vector<base::FilePath>& temporary_files,
    base::OnceCallback<void(std::vector<blink::mojom::FileChooserFileInfoPtr>)>
        notify,
    StripResult result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // Hand the temporary copies to FileSelectHelper so they are deleted once the
  // upload finishes, via its existing DeleteTemporaryFiles() teardown.
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

  // Base level checks.
  if (!base::FeatureList::IsEnabled(
          image_metadata_stripper::features::kStripImageMetadataV1) ||
      !HasStrippableImage(list)) {
    return false;
  }

  // Second pass, re-entered with the sanitized list: let the upstream path run.
  if (already_processed) {
    return false;
  }

  already_processed = true;

  // I/O-blocking copy + scrub off the UI thread. `notify` keeps the
  // FileSelectHelper alive across the hop, so the `temporary_files` reference
  // handed to the reply stays valid.
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

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/download/brave_download_manager_delegate.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/supports_user_data.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
#include "chrome/browser/profiles/profile.h"
#include "components/download/public/common/download_item.h"

namespace {

// Tracks whether a download's image metadata still needs to be stripped, so the
// blocking work runs at most once even though IsDownloadReadyForCompletion is
// invoked repeatedly. Mirrors enterprise_obfuscation::DownloadObfuscationData.
class ImageMetadataStripData : public base::SupportsUserData::Data {
 public:
  static const void* const kUserDataKey;

  bool needs_stripping = true;
};

// static
const void* const ImageMetadataStripData::kUserDataKey =
    &ImageMetadataStripData::kUserDataKey;

}  // namespace

BraveDownloadManagerDelegate::BraveDownloadManagerDelegate(Profile* profile)
    : ChromeDownloadManagerDelegate(profile) {}

BraveDownloadManagerDelegate::~BraveDownloadManagerDelegate() = default;

bool BraveDownloadManagerDelegate::IsDownloadReadyForCompletion(
    download::DownloadItem* item,
    base::OnceClosure internal_complete_callback) {
  // Retain a copy of the callback so completion can be re-triggered once
  // metadata stripping finishes, even when the base class reports the download
  // as ready.
  auto split_callback =
      base::SplitOnceCallback(std::move(internal_complete_callback));
  if (!ChromeDownloadManagerDelegate::IsDownloadReadyForCompletion(
          item, std::move(split_callback.first))) {
    return false;
  }

  const std::string mime_type = item->GetMimeType();
  if (mime_type != "image/png" && mime_type != "image/jpeg") {
    return true;
  }

  ImageMetadataStripData* strip_data = static_cast<ImageMetadataStripData*>(
      item->GetUserData(ImageMetadataStripData::kUserDataKey));
  if (!strip_data) {
    strip_data = new ImageMetadataStripData();
    item->SetUserData(ImageMetadataStripData::kUserDataKey,
                      base::WrapUnique(strip_data));
  }

  if (strip_data->needs_stripping) {
    // RemoveIptcMetadata() performs blocking file I/O, so it must run on a
    // thread pool sequence that permits blocking rather than the UI thread.
    // Completion is deferred until it finishes so the download is not finalized
    // before the tracking metadata is removed.
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE,
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
        base::BindOnce(&image_metadata_stripper::RemoveIptcMetadata,
                       item->GetFullPath()),
        base::BindOnce(&BraveDownloadManagerDelegate::OnImageMetadataStripped,
                       weak_ptr_factory_.GetWeakPtr(),
                       std::move(split_callback.second)));

    // Ensure that stripping is run only once.
    strip_data->needs_stripping = false;
    return false;
  }

  return true;
}

void BraveDownloadManagerDelegate::OnImageMetadataStripped(
    base::OnceClosure internal_complete_callback,
    bool success) {
  if (!success) {
    DVLOG(1) << "Failed to strip image metadata from download file.";
  }

  if (internal_complete_callback) {
    std::move(internal_complete_callback).Run();
  }
}

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/download/brave_download_manager_delegate.h"

#include <memory>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
#include "chrome/browser/profiles/profile.h"
#include "components/download/public/common/download_item.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_manager.h"

namespace {

void LogStrippingResult(
    const image_metadata_stripper::StrippingResultCode result) {
  // Debug logs.
  switch (result) {
    case image_metadata_stripper::StrippingResultCode::kFileNotFound: {
      DVLOG(1) << "Stripping skipped as the download file does not exist.";
      return;
    }

    case image_metadata_stripper::StrippingResultCode::kFileReadFailed: {
      DVLOG(1) << "Failed to read the download file to check for metadata.";
      return;
    }

    case image_metadata_stripper::StrippingResultCode::kFileWriteFailed: {
      DVLOG(1) << "Failed to rewrite the download file without the metadata.";
      return;
    }

    case image_metadata_stripper::StrippingResultCode::kMetadataNotFound: {
      DVLOG(1) << "Stripping ignored as FBMD metadata may not be present.";
      return;
    }

    case image_metadata_stripper::StrippingResultCode::kStrippingFailed: {
      DVLOG(1) << "Failed to strip image metadata from download file.";
      return;
    }

    case image_metadata_stripper::StrippingResultCode::kStripped: {
      DVLOG(1) << "FBMD found and stripped from image metadata.";
      return;
    }
  }
  NOTREACHED();
}
}  // namespace

// Factory used by the plastered download_core_service_impl.cc construction
// site. Keeping the Brave header out of chrome/browser/download avoids pulling
// Brave into that target's dependency graph.
std::unique_ptr<ChromeDownloadManagerDelegate>
CreateBraveDownloadManagerDelegate(Profile* profile) {
  return std::make_unique<BraveDownloadManagerDelegate>(profile);
}

BraveDownloadManagerDelegate::BraveDownloadManagerDelegate(Profile* profile)
    : ChromeDownloadManagerDelegate(profile) {}

BraveDownloadManagerDelegate::~BraveDownloadManagerDelegate() = default;

bool BraveDownloadManagerDelegate::IsDownloadReadyForCompletion(
    download::DownloadItem* item,
    base::OnceClosure internal_complete_callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!base::FeatureList::IsEnabled(
          image_metadata_stripper::features::kStripImageMetadataV1)) {
    return ChromeDownloadManagerDelegate::IsDownloadReadyForCompletion(
        item, std::move(internal_complete_callback));
  }

  // IPTC metadata stripping only available for jpeg types. So, for non
  // types rely on upstream flow.
  // TODO(https://github.com/brave/brave-browser/issues/5238): PNG formats needs
  // more investigation whether FBMD is present or not. So, tackling only jpeg.
  const std::string mime_type = item->GetMimeType();
  if (mime_type != "image/jpeg") {
    return ChromeDownloadManagerDelegate::IsDownloadReadyForCompletion(
        item, std::move(internal_complete_callback));
  }

  // IPTC metadata stripping begins. This state similar to upstream
  // SafeBrowsingState, helps to let the callers know when the metadata
  // stripping is completed to unblock the download flow.
  if (IptcStrippingState* state = static_cast<IptcStrippingState*>(
          item->GetUserData(IptcStrippingState::kUserDataKey))) {
    // Second pass after OnImageMetadataStripped(): stripping only starts after
    // upstream already returned true, so do not call upstream again.
    if (state->is_complete()) {
      return true;
    }

    // Stripping already in flight: refresh the resume callback and keep
    // blocking. Upstream has already returned ready on a prior pass.
    if (state->stripping_started) {
      state->set_callback(std::move(internal_complete_callback));
      return false;
    }

    NOTREACHED() << "Stripping has not started but we somehow created a valid "
                    "IptcStrippingState.";
  }

  // First pass for a strippable image: require upstream readiness, then take
  // over completion for metadata stripping.
  auto [upstream_check_complete_callback, iptc_stripping_complete_callback] =
      base::SplitOnceCallback(std::move(internal_complete_callback));
  if (!ChromeDownloadManagerDelegate::IsDownloadReadyForCompletion(
          item, std::move(upstream_check_complete_callback))) {
    return false;
  }

  auto state = std::make_unique<IptcStrippingState>();
  state->set_callback(std::move(iptc_stripping_complete_callback));
  state->stripping_started = true;
  item->SetUserData(IptcStrippingState::kUserDataKey, std::move(state));

  // I/O-blocking IPTC scrub off the UI thread.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&image_metadata_stripper::RemoveIptcMetadata,
                     item->GetFullPath()),
      base::BindOnce(&BraveDownloadManagerDelegate::OnImageMetadataStripped,
                     weak_ptr_factory_.GetWeakPtr(), item->GetId()));
  return false;
}

void BraveDownloadManagerDelegate::OnImageMetadataStripped(
    uint32_t download_id,
    image_metadata_stripper::StrippingResultCode result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  LogStrippingResult(result);

  // The download may have been removed while the stripping task was running, so
  // the item and its keyed state have to be looked up again.
  if (download_manager_) {
    download::DownloadItem* item = download_manager_->GetDownload(download_id);
    if (item) {
      IptcStrippingState* state = static_cast<IptcStrippingState*>(
          item->GetUserData(IptcStrippingState::kUserDataKey));
      if (state && !state->is_complete()) {
        DCHECK(state->stripping_started);
        state->CompleteDownload();
      }
    }
  }
}

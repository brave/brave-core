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
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/supports_user_data.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
#include "chrome/browser/profiles/profile.h"
#include "components/download/public/common/download_item.h"
#include "content/public/browser/browser_thread.h"

BraveDownloadManagerDelegate::BraveDownloadManagerDelegate(Profile* profile)
    : ChromeDownloadManagerDelegate(profile) {}

BraveDownloadManagerDelegate::~BraveDownloadManagerDelegate() = default;

bool BraveDownloadManagerDelegate::IsDownloadReadyForCompletion(
    download::DownloadItem* item,
    base::OnceClosure internal_complete_callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!base::FeatureList::IsEnabled(
          image_metadata_stripper::features::kStripDownloadedImageMetadata)) {
    return ChromeDownloadManagerDelegate::IsDownloadReadyForCompletion(
        item, std::move(internal_complete_callback));
  }

  // |chromium_callback| is fired only when we return true from here and
  // |brave_callback| is not fired.
  auto [chromium_callback, brave_callback] =
      base::SplitOnceCallback(std::move(internal_complete_callback));
  if (!ChromeDownloadManagerDelegate::IsDownloadReadyForCompletion(
          item, std::move(chromium_callback))) {
    return false;
  }

  // Perform iptc scrubbing on jpeg/png formats.
  const std::string mime_type = item->GetMimeType();
  if (mime_type != "image/png" && mime_type != "image/jpeg") {
    return true;
  }

  // This keyed state helps to ensure we do the stripping only once.
  IptcStrippingState* state = static_cast<IptcStrippingState*>(
      item->GetUserData(IptcStrippingState::kUserDataKey));
  if (!state) {
    state = new IptcStrippingState();
    state->set_callback(std::move(brave_callback));
    item->SetUserData(IptcStrippingState::kUserDataKey,
                      base::WrapUnique(state));
  }

  if (!state->stripping_started) {
    // Initiate the stripping which is a I/O blocking task into a separate
    // thread which is not UI.
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE,
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
        base::BindOnce(&image_metadata_stripper::RemoveIptcMetadata,
                       item->GetFullPath()),
        base::BindOnce(&BraveDownloadManagerDelegate::OnImageMetadataStripped,
                       weak_ptr_factory_.GetWeakPtr(), state));

    state->stripping_started = true;
    return false;
  }

  return true;
}

void BraveDownloadManagerDelegate::OnImageMetadataStripped(
    IptcStrippingState* state,
    bool success) {
  if (!success) {
    DVLOG(1) << "Failed to strip image metadata from download file.";
  }

  if (state && !state->is_complete()) {
    DCHECK(state->stripping_started);
    state->CompleteDownload();
  }
}

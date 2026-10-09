/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/drag_drop/brave_drag_drop_image_metadata_stripper.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/image_metadata_stripper/file_upload_controller.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
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

// The callback which gets fired after all the stripping was completed.
void OnStripComplete(DropCompletionCallback callback,
                     content::DropData drop_data,
                     std::vector<std::optional<base::FilePath>> copies) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK_EQ(drop_data.filenames.size(), copies.size());

  for (size_t i = 0; i < copies.size(); ++i) {
    if (copies[i]) {
      drop_data.filenames[i].path = *std::move(copies[i]);
    }
  }
  std::move(callback).Run(std::move(drop_data));
}

void MaybeStripDropData(base::WeakPtr<content::WebContents> web_contents,
                        DropCompletionCallback callback,
                        std::optional<content::DropData> drop_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // The controller is looked up only now, since the tab may have closed while
  // the drop was pending.
  auto* controller =
      image_metadata_stripper::FileUploadController::FromWebContents(
          web_contents.get());

  // Nothing to do when the drop was blocked upstream, when the tab it targets
  // is gone, or when no dropped file could carry the metadata we strip.
  if (!drop_data || !controller || !HasSupportedImage(*drop_data)) {
    std::move(callback).Run(std::move(drop_data));
    return;
  }

  // Stripping begins.
  std::vector<base::FilePath> paths;
  paths.reserve(drop_data->filenames.size());
  for (const ui::FileInfo& file : drop_data->filenames) {
    paths.push_back(file.path);
  }
  controller->Strip(std::move(paths),
                    image_metadata_stripper::StrippingClient::kDragDrop,
                    base::BindOnce(&OnStripComplete, std::move(callback),
                                   std::move(*drop_data)));
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

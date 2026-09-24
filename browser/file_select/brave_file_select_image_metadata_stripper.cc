/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/file_select/brave_file_select_image_metadata_stripper.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/browser/image_metadata_stripper/image_metadata_stripper_upload_controller.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom.h"

namespace brave {

namespace {

// IN-TEST
base::OnceClosure* g_on_strip_completed_callback_for_testing_ = nullptr;

std::vector<base::FilePath> NativePathsFromList(
    const std::vector<blink::mojom::FileChooserFileInfoPtr>& list) {
  std::vector<base::FilePath> files;
  files.reserve(list.size());
  for (const auto& info : list) {
    files.push_back((info && info->is_native_file())
                        ? info->get_native_file()->file_path
                        : base::FilePath());
  }
  return files;
}

void OnStripComplete(
    base::OnceCallback<void(std::vector<blink::mojom::FileChooserFileInfoPtr>)>
        notify,
    std::vector<blink::mojom::FileChooserFileInfoPtr> selected_files,
    std::vector<std::optional<base::FilePath>> copies) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK_EQ(selected_files.size(), copies.size());

  for (size_t i = 0; i < copies.size(); ++i) {
    if (!copies[i]) {
      continue;
    }
    CHECK(selected_files[i] && selected_files[i]->is_native_file());
    auto& native = selected_files[i]->get_native_file();
    if (native->display_name.empty()) {
      native->display_name = native->file_path.BaseName().AsUTF16Unsafe();
    }
    native->file_path = *copies[i];
  }

  if (g_on_strip_completed_callback_for_testing_) {
    CHECK_IS_TEST();
    CHECK(!g_on_strip_completed_callback_for_testing_->is_null());
    std::move(*g_on_strip_completed_callback_for_testing_).Run();
    g_on_strip_completed_callback_for_testing_ = nullptr;
  }

  std::move(notify).Run(std::move(selected_files));
}

}  // namespace

bool MaybeStripImageMetadataForUpload(
    bool& already_processed,
    content::WebContents* web_contents,
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

  ImageMetadataStripperUploadController* controller =
      ImageMetadataStripperUploadController::From(web_contents);
  std::vector<base::FilePath> files = NativePathsFromList(list);
  if (!controller ||
      !ImageMetadataStripperUploadController::ContainsSupportedImage(files)) {
    return false;
  }

  already_processed = true;

  controller->MaybeStrip(
      image_metadata_stripper::StrippingClient::kFileSelect, std::move(files),
      base::BindOnce(&OnStripComplete, std::move(notify), std::move(list)));
  return true;
}

void SetStripCompletedCallbackForTesting(  // IN-TEST
    base::OnceClosure* callback) {
  g_on_strip_completed_callback_for_testing_ = callback;
}

}  // namespace brave

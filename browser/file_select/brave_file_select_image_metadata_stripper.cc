/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/file_select/brave_file_select_image_metadata_stripper.h"

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/check_op.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/browser/image_metadata_stripper/file_upload_controller.h"
#include "brave/components/image_metadata_stripper/common/features.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom.h"

namespace brave {

namespace {

// IN-TEST
base::OnceCallback<void(std::vector<base::FilePath>)>*
    g_on_strip_completed_callback_for_testing_ = nullptr;

// Non-native entries map to an empty path, which yields no copy.
std::vector<base::FilePath> GetNativeFilePaths(
    const std::vector<blink::mojom::FileChooserFileInfoPtr>& selected_files) {
  std::vector<base::FilePath> paths;
  paths.reserve(selected_files.size());
  for (const auto& info : selected_files) {
    paths.push_back(info && info->is_native_file()
                        ? info->get_native_file()->file_path
                        : base::FilePath());
  }
  return paths;
}

// The callback which gets fired after all the stripping was completed.
void OnStripComplete(
    base::OnceCallback<void(std::vector<blink::mojom::FileChooserFileInfoPtr>)>
        notify,
    std::vector<blink::mojom::FileChooserFileInfoPtr> selected_files,
    std::vector<std::optional<base::FilePath>> copies) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK_EQ(selected_files.size(), copies.size());

  for (size_t i = 0; i < selected_files.size(); ++i) {
    if (!copies[i]) {
      continue;
    }

    // We are swapping out the path of the original file with the copy which has
    // been stripped of metadata. On macOS file controls show the name the user
    // picked, and the copy carries the same basename.
    auto& native = selected_files[i]->get_native_file();
    if (native->display_name.empty()) {
      native->display_name = native->file_path.BaseName().AsUTF16Unsafe();
    }
    native->file_path = *std::move(copies[i]);
  }

  // Test guarded code to get the list of temporary files we created from our
  // component.
  if (g_on_strip_completed_callback_for_testing_) {
    CHECK_IS_TEST();
    CHECK(!g_on_strip_completed_callback_for_testing_->is_null());
    g_on_strip_completed_callback_for_testing_ = nullptr;
  }

  std::move(notify).Run(std::move(selected_files));
}

}  // namespace

bool MaybeStripImageMetadataForUpload(
    content::WebContents* web_contents,
    bool& already_processed,
    std::vector<blink::mojom::FileChooserFileInfoPtr>& list,
    base::OnceCallback<void(std::vector<blink::mojom::FileChooserFileInfoPtr>)>
        notify) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!web_contents || list.empty() ||
      !base::FeatureList::IsEnabled(
          image_metadata_stripper::features::kStripImageMetadataV1)) {
    return false;
  }

  // A flag to ensure we don't have infinite loops between the caller and
  // callee once the metadata removal task get posted and the instruction
  // pointer returns back to the caller.
  if (already_processed) {
    return false;
  }

  auto* controller =
      image_metadata_stripper::FileUploadController::FromWebContents(
          web_contents);
  if (!controller) {
    return false;
  }

  already_processed = true;

  // Stripping begins.
  std::vector<base::FilePath> paths = GetNativeFilePaths(list);
  controller->Strip(
      std::move(paths), image_metadata_stripper::StrippingClient::kFileSelect,
      base::BindOnce(&OnStripComplete, std::move(notify), std::move(list)));
  return true;
}

void SetStripCompletedCallbackForTesting(  // IN-TEST
    base::OnceCallback<void(std::vector<base::FilePath>)>* callback) {
  g_on_strip_completed_callback_for_testing_ = callback;
}

}  // namespace brave

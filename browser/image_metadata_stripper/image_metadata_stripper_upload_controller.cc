/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/image_metadata_stripper/image_metadata_stripper_upload_controller.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/thread_pool.h"
#include "build/build_config.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/android/brave_tab_features.h"
#else
#include "brave/browser/ui/tabs/public/brave_tab_features.h"
#endif

namespace brave {

namespace {

constexpr base::FilePath::CharType kStripTempDirPrefix[] =
    FILE_PATH_LITERAL("brave_image_strip");

// Tries to strip the metadata from the image at file path |src|.
// Returns an optional file path pointing to the new temporary stripped out
// image if stripped. This method doesn't take care of cleaning up the temporary
// files.
std::optional<base::FilePath> MaybeStripOnBlockingThread(
    const base::FilePath& temp_root_dir,
    size_t sub_dir_index,
    image_metadata_stripper::StrippingClient client,
    const base::FilePath& src) {
  // "" or "../" cannot name the copy, and the latter could escape the isolation
  // the temporary root directory provides.
  const base::FilePath basename = src.BaseName();
  if (basename.empty() || basename.ReferencesParent()) {
    return std::nullopt;
  }

  // A stripped image always sits inside its own sub directory.
  base::ScopedTempDir sub_dir;
  if (!sub_dir.Set(
          temp_root_dir.AppendASCII(base::NumberToString(sub_dir_index)))) {
    LOG(ERROR) << "Image strip skipped; temp subdir could not be created: "
               << src;
    return std::nullopt;
  }

  const base::FilePath copy = sub_dir.GetPath().Append(basename);
  if (!base::CopyFile(src, copy)) {
    DVLOG(1) << "Image strip skipped; failed to copy the image file to a "
                "temporary file.";
    return std::nullopt;
  }

  if (!image_metadata_stripper::RemoveIptcMetadata(client, copy)) {
    DVLOG(1) << "No stripping occured; keeping original: " << src;
    return std::nullopt;
  }

  sub_dir.Take();
  return copy;
}

}  // namespace

ImageMetadataStripperUploadController::RootState::RootState() = default;
ImageMetadataStripperUploadController::RootState::~RootState() = default;

// static
ImageMetadataStripperUploadController*
ImageMetadataStripperUploadController::From(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return nullptr;
  }

  tabs::TabInterface* tab =
      tabs::TabInterface::MaybeGetFromContents(web_contents);
  if (!tab) {
    return nullptr;
  }

  tabs::TabFeatures* tab_features = tab->GetTabFeatures();
  if (!tab_features) {
    return nullptr;
  }

  tabs::BraveTabFeatures* brave_tab_features =
      tabs::BraveTabFeatures::FromTabFeatures(tab_features);
  if (!brave_tab_features) {
    return nullptr;
  }

  return brave_tab_features->image_metadata_stripper_dir_controller();
}

// static
bool ImageMetadataStripperUploadController::ContainsSupportedImage(
    const std::vector<base::FilePath>& files) {
  return std::ranges::any_of(files,
                             &image_metadata_stripper::IsSupportedImagePath);
}

ImageMetadataStripperUploadController::ImageMetadataStripperUploadController(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      blocking_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      state_(base::MakeRefCounted<RootState>()) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK(web_contents);
}

ImageMetadataStripperUploadController::
    ~ImageMetadataStripperUploadController() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // `state_->root` is a ScopedTempDir. Drop the last ref on the sequence
  // that created it, after any in-flight strip, so the destructor deletes
  // the directory off the UI thread.
  blocking_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce([](scoped_refptr<RootState>) {}, std::move(state_)));
}

void ImageMetadataStripperUploadController::MaybeStrip(
    image_metadata_stripper::StrippingClient client,
    std::vector<base::FilePath> files,
    StrippedCopiesCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK(callback);

  if (files.empty() || !web_contents() || !ContainsSupportedImage(files)) {
    std::move(callback).Run(
        std::vector<std::optional<base::FilePath>>(files.size()));
    return;
  }

  blocking_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(
          &ImageMetadataStripperUploadController::StripOnBlockingThread, state_,
          client, std::move(files)),
      base::BindOnce(&ImageMetadataStripperUploadController::OnStripComplete,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

base::FilePath ImageMetadataStripperUploadController::GetTempRootDirForTesting()
    const {
  return temp_root_dir_for_testing_;
}

// static
ImageMetadataStripperUploadController::StripResult
ImageMetadataStripperUploadController::StripOnBlockingThread(
    scoped_refptr<RootState> state,
    image_metadata_stripper::StrippingClient client,
    std::vector<base::FilePath> files) {
  StripResult result;
  result.copies.reserve(files.size());

  for (const base::FilePath& file : files) {
    if (file.empty() || !image_metadata_stripper::IsSupportedImagePath(file) ||
        !image_metadata_stripper::ContainsMetadataToStrip(file)) {
      result.copies.emplace_back(std::nullopt);
      continue;
    }

    // Root is created only once we know a strip will be attempted.
    if (!state->root.IsValid() &&
        !state->root.CreateUniqueTempDir(kStripTempDirPrefix)) {
      LOG(ERROR) << "Image strip skipped; temp directory could not be created.";
      result.copies.emplace_back(std::nullopt);
      continue;
    }

    result.copies.push_back(MaybeStripOnBlockingThread(
        state->root.GetPath(), state->next_index++, client, file));
  }

  result.temp_root_dir =
      state->root.IsValid() ? state->root.GetPath() : base::FilePath();
  return result;
}

void ImageMetadataStripperUploadController::OnStripComplete(
    StrippedCopiesCallback callback,
    StripResult result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (temp_root_dir_for_testing_.empty()) {
    temp_root_dir_for_testing_ = result.temp_root_dir;
  }

  std::move(callback).Run(std::move(result.copies));
}

}  // namespace brave

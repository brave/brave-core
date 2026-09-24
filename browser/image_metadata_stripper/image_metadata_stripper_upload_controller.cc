/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/image_metadata_stripper/image_metadata_stripper_upload_controller.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/thread_pool.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/page.h"
#include "content/public/browser/page_user_data.h"
#include "content/public/browser/web_contents.h"

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

class PageLifetime : public content::PageUserData<PageLifetime> {
 public:
  ~PageLifetime() override;

 private:
  friend PageUserData;
  PAGE_USER_DATA_KEY_DECL();

  PageLifetime(content::Page& page, base::OnceClosure on_destroy);

  base::OnceClosure on_destroy_;
};

PAGE_USER_DATA_KEY_IMPL(PageLifetime);

PageLifetime::PageLifetime(content::Page& page, base::OnceClosure on_destroy)
    : PageUserData(page), on_destroy_(std::move(on_destroy)) {}

PageLifetime::~PageLifetime() {
  if (on_destroy_) {
    std::move(on_destroy_).Run();
  }
}

}  // namespace

DEFINE_USER_DATA(ImageMetadataStripperUploadController);

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

  return Get(tab->GetUnownedUserDataHost());
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
      state_(std::make_unique<RootState>()) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK(web_contents);

  if (tabs::TabInterface* tab =
          tabs::TabInterface::MaybeGetFromContents(web_contents)) {
    scoped_unowned_user_data_.emplace(tab->GetUnownedUserDataHost(), *this);
  }
}

ImageMetadataStripperUploadController::
    ~ImageMetadataStripperUploadController() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // `state_->root` is a ScopedTempDir. Destroy it on the sequence that
  // created it, after any in-flight strip, so the destructor does not
  // delete on the UI thread.
  blocking_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce([](std::unique_ptr<RootState>) {}, std::move(state_)));
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

  EnsureBoundToPrimaryPage();

  // Same sequence later takes ownership of `state_` (dtor / ResetTempRoot),
  // so this pointer stays valid until already-posted strip tasks finish.
  blocking_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(
          &ImageMetadataStripperUploadController::StripOnBlockingThread,
          base::Unretained(state_.get()), client, std::move(files)),
      std::move(callback));
}

base::FilePath ImageMetadataStripperUploadController::GetTempRootDirForTesting()
    const {
  CHECK(state_);
  return state_->path;
}

// static
std::vector<std::optional<base::FilePath>>
ImageMetadataStripperUploadController::StripOnBlockingThread(
    RootState* state,
    image_metadata_stripper::StrippingClient client,
    std::vector<base::FilePath> files) {
  CHECK(state);

  std::vector<std::optional<base::FilePath>> copies;
  copies.reserve(files.size());

  for (const base::FilePath& file : files) {
    if (file.empty() || !image_metadata_stripper::IsSupportedImagePath(file) ||
        !image_metadata_stripper::ContainsMetadataToStrip(file)) {
      copies.emplace_back(std::nullopt);
      continue;
    }

    // Root is created only once we know a strip will be attempted.
    if (state->path.empty()) {
      if (!state->root.CreateUniqueTempDir(kStripTempDirPrefix)) {
        LOG(ERROR)
            << "Image strip skipped; temp directory could not be created.";
        copies.emplace_back(std::nullopt);
        continue;
      }
      state->path = state->root.GetPath();
    }

    copies.push_back(MaybeStripOnBlockingThread(
        state->root.GetPath(), state->next_index++, client, file));
  }

  return copies;
}

void ImageMetadataStripperUploadController::EnsureBoundToPrimaryPage() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK(web_contents());

  content::Page& page = web_contents()->GetPrimaryPage();
  if (PageLifetime::GetForPage(page)) {
    return;
  }

  PageLifetime::CreateForPage(
      page,
      base::BindOnce(&ImageMetadataStripperUploadController::ResetTempRoot,
                     weak_factory_.GetWeakPtr()));
}

void ImageMetadataStripperUploadController::ResetTempRoot() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  blocking_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce([](std::unique_ptr<RootState>) {}, std::move(state_)));
  state_ = std::make_unique<RootState>();
}

}  // namespace brave

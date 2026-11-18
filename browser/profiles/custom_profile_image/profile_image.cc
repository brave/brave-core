// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/profiles/custom_profile_image/profile_image.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/supports_user_data.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/browser/ui/webui/custom_profile_image/features.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "third_party/skia/include/core/SkImageInfo.h"

namespace custom_profile_image {
namespace {
constexpr char kSaved[] = "brave_custom_image_saved";
constexpr char kActive[] = "brave_custom_image_active";
}  // namespace

// static
ProfileImage& ProfileImage::Get(ProfileAttributesEntry& entry) {
  std::unique_ptr<base::SupportsUserData::Data>& image =
      entry.custom_profile_image();
  if (!image) {
    image = base::WrapUnique(new ProfileImage(entry));
  }
  return static_cast<ProfileImage&>(*image);
}

bool ProfileImage::IsEnabled() {
  return base::FeatureList::IsEnabled(features::kBraveCustomProfileImage);
}

ProfileImage::~ProfileImage() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!has_committed_image_) {
    // The entry is being destroyed, so this is tracked locally rather than
    // read from its prefs.
    file_.AsyncCall(base::IgnoreResult(&ImageFile::Remove));
  }
}

bool ProfileImage::HasSavedImage() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return entry_->GetCustomProfileImageFlag(kSaved);
}

bool ProfileImage::IsActive() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return IsEnabled() && HasSavedImage() &&
         entry_->GetCustomProfileImageFlag(kActive);
}

bool ProfileImage::IsSelected() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return IsActive() && !load_failed_;
}

const gfx::Image& ProfileImage::GetImage() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (HasSavedImage() && !loaded_ && !loading_) {
    loading_ = true;
    file_.AsyncCall(&ImageFile::Read)
        .Then(
            base::BindOnce(&ProfileImage::Loaded, weak_factory_.GetWeakPtr()));
  }
  return image_;
}

void ProfileImage::Replace(const SkBitmap& bitmap, Callback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!IsEnabled()) {
    ReplyLater(std::move(callback), Result::kUnavailable);
    return;
  }
  // Reject here rather than on the file sequence, so a bad bitmap is never
  // copied or sent.
  if (bitmap.drawsNothing() || bitmap.width() != bitmap.height() ||
      bitmap.width() > kMaxImageSide) {
    ReplyLater(std::move(callback), Result::kInvalidImage);
    return;
  }
  // The file sequence gets its own immutable copy, converted to the format a
  // later read decodes to. Copying fails if the pixels cannot be read.
  SkBitmap copy;
  if (!copy.tryAllocPixels(
          SkImageInfo::MakeN32Premul(bitmap.width(), bitmap.height())) ||
      !bitmap.readPixels(copy.pixmap(), 0, 0)) {
    ReplyLater(std::move(callback), Result::kInvalidImage);
    return;
  }
  copy.setImmutable();
  const uint64_t generation = generation_;
  file_.AsyncCall(&ImageFile::Replace)
      .WithArgs(copy)
      .Then(base::BindOnce(&ProfileImage::Replaced, weak_factory_.GetWeakPtr(),
                           generation, gfx::Image::CreateFrom1xBitmap(copy),
                           std::move(callback)));
}

void ProfileImage::Remove(Callback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!IsEnabled()) {
    ReplyLater(std::move(callback), Result::kUnavailable);
    return;
  }
  file_.AsyncCall(&ImageFile::Remove)
      .Then(base::BindOnce(&ProfileImage::Removed, weak_factory_.GetWeakPtr(),
                           std::move(callback)));
}

void ProfileImage::Select(Callback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!IsEnabled() || !HasSavedImage()) {
    ReplyLater(std::move(callback), Result::kUnavailable);
    return;
  }
  // Recheck the file so a missing or damaged saved image is recoverable.
  file_.AsyncCall(&ImageFile::Read)
      .Then(base::BindOnce(&ProfileImage::Selected, weak_factory_.GetWeakPtr(),
                           generation_, std::move(callback)));
}

void ProfileImage::SelectFallback() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Overrides a pending Replace or Select even before anything is saved.
  ++generation_;
  // Record preset choices made while the feature is off so re-enabling it
  // does not restore the custom image over the user's later choice. Without a
  // saved image the flag means nothing, so Local State is left alone.
  if (HasSavedImage() && entry_->SetCustomProfileImageFlag(kActive, false)) {
    Notify();
  }
}

ProfileImage::ProfileImage(ProfileAttributesEntry& entry)
    : file_(base::ThreadPool::CreateSequencedTaskRunner(
                {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
                 base::TaskShutdownBehavior::BLOCK_SHUTDOWN}),
            entry.GetPath()),
      has_committed_image_(entry.GetCustomProfileImageFlag(kSaved)),
      entry_(entry) {
  if (!has_committed_image_) {
    // A first write interrupted before metadata publication is not committed.
    file_.AsyncCall(base::IgnoreResult(&ImageFile::Remove));
  }
}

void ProfileImage::Loaded(SkBitmap bitmap) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  loading_ = false;
  // A Replace, Remove or Select that finished first already set newer state.
  if (loaded_) {
    return;
  }
  loaded_ = true;
  load_failed_ = bitmap.drawsNothing();
  image_ = gfx::Image::CreateFrom1xBitmap(bitmap);
  Notify();
}

void ProfileImage::Replaced(uint64_t generation,
                            gfx::Image image,
                            Callback callback,
                            bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!success) {
    std::move(callback).Run(Result::kFileError);
    return;
  }
  image_ = std::move(image);
  loaded_ = true;
  load_failed_ = false;
  has_committed_image_ = true;
  entry_->SetCustomProfileImageFlag(kSaved, true);
  if (generation == generation_) {
    entry_->SetCustomProfileImageFlag(kActive, true);
  }
  Notify();
  std::move(callback).Run(Result::kSuccess);
}

void ProfileImage::Removed(Callback callback, bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!success) {
    std::move(callback).Run(Result::kFileError);
    return;
  }
  image_ = gfx::Image();
  loaded_ = true;
  load_failed_ = false;
  has_committed_image_ = false;
  const bool saved_changed = entry_->SetCustomProfileImageFlag(kSaved, false);
  const bool active_changed = entry_->SetCustomProfileImageFlag(kActive, false);
  if (saved_changed || active_changed) {
    Notify();
  }
  std::move(callback).Run(Result::kSuccess);
}

void ProfileImage::Selected(uint64_t generation,
                            Callback callback,
                            SkBitmap bitmap) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (bitmap.drawsNothing()) {
    if (generation == generation_) {
      image_ = gfx::Image();
      loaded_ = true;
      load_failed_ = true;
      Notify();
    }
    std::move(callback).Run(Result::kFileError);
    return;
  }
  if (generation == generation_) {
    image_ = gfx::Image::CreateFrom1xBitmap(bitmap);
    loaded_ = true;
    load_failed_ = false;
    entry_->SetCustomProfileImageFlag(kActive, true);
    Notify();
  }
  std::move(callback).Run(Result::kSuccess);
}

// Early results are posted so every callback runs asynchronously, like the
// ones that wait for the file sequence.
void ProfileImage::ReplyLater(Callback callback, Result result) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&ProfileImage::Reply, weak_factory_.GetWeakPtr(),
                     std::move(callback), result));
}

void ProfileImage::Reply(Callback callback, Result result) {
  std::move(callback).Run(result);
}

void ProfileImage::Notify() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  entry_->NotifyCustomProfileImageChanged();
}

}  // namespace custom_profile_image

// Hooks called from upstream profile code.
gfx::Image BraveGetCustomProfileImage(const ProfileAttributesEntry& entry) {
  if (!custom_profile_image::ProfileImage::IsEnabled()) {
    return {};
  }
  // Upstream callers are const; the image state is a cache.
  auto& image = custom_profile_image::ProfileImage::Get(
      const_cast<ProfileAttributesEntry&>(entry));
  return image.IsActive() ? image.GetImage() : gfx::Image();
}

bool BraveIsCustomProfileImageSelected(const ProfileAttributesEntry& entry) {
  return custom_profile_image::ProfileImage::IsEnabled() &&
         custom_profile_image::ProfileImage::Get(
             const_cast<ProfileAttributesEntry&>(entry))
             .IsSelected();
}

void BraveSelectFallbackProfileImage(ProfileAttributesEntry& entry) {
  custom_profile_image::ProfileImage::Get(entry).SelectFallback();
}

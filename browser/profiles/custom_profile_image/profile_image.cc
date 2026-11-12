// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/profiles/custom_profile_image/profile_image.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/supports_user_data.h"
#include "base/task/thread_pool.h"
#include "brave/browser/ui/webui/custom_profile_image/features.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"

namespace custom_profile_image {
namespace {
constexpr char kSaved[] = "brave_custom_image_saved";
constexpr char kActive[] = "brave_custom_image_active";

// Early results are posted so every callback runs asynchronously, like the
// ones that wait for the file sequence.
void ReplyLater(ProfileImage::Callback callback, Result result) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), result));
}
}  // namespace

// Held by ProfileAttributesStorage, so removal and reuse of a profile path
// share one file sequence.
class ProfileImage::Registry : public base::SupportsUserData::Data {
 public:
  static Registry& Get(ProfileAttributesStorage& storage) {
    if (!storage.custom_profile_images_) {
      storage.custom_profile_images_ = std::make_unique<Registry>();
    }
    return static_cast<Registry&>(*storage.custom_profile_images_);
  }

  ProfileImage& Get(ProfileAttributesEntry& entry) {
    std::unique_ptr<ProfileImage>& image = images_[entry.GetPath()];
    if (!image) {
      image = base::WrapUnique(new ProfileImage(entry, file_runner_));
    }
    return *image;
  }

  void Remove(const base::FilePath& path) {
    // Runs after accepted writes, without recreating the profile directory.
    // This is not ordered with upstream's directory deletion, which runs on
    // another sequence, retries on failure, and removes any file left here.
    base::SequenceBound<ImageFile>(file_runner_, path)
        .AsyncCall(base::IgnoreResult(&ImageFile::Remove));
    images_.erase(path);
  }

 private:
  const scoped_refptr<base::SequencedTaskRunner> file_runner_ =
      base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  base::flat_map<base::FilePath, std::unique_ptr<ProfileImage>> images_;
};

ProfileImage& ProfileImage::Get(ProfileAttributesEntry& entry) {
  return Registry::Get(*entry.profile_attributes_storage_).Get(entry);
}

// static
void ProfileImage::ProfileRemoved(ProfileAttributesStorage& storage,
                                  const base::FilePath& path) {
  Registry::Get(storage).Remove(path);
}

bool ProfileImage::IsEnabled() {
  return base::FeatureList::IsEnabled(features::kBraveCustomProfileImage);
}

ProfileImage::ProfileImage(ProfileAttributesEntry& entry,
                           scoped_refptr<base::SequencedTaskRunner> file_runner)
    : file_(std::move(file_runner), entry.GetPath()),
      has_committed_image_(entry.GetBool(kSaved)),
      storage_(*entry.profile_attributes_storage_),
      path_(entry.GetPath()) {
  if (!has_committed_image_) {
    // A first write interrupted before metadata publication is not committed.
    file_.AsyncCall(base::IgnoreResult(&ImageFile::Remove));
  }
}

ProfileImage::~ProfileImage() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!has_committed_image_) {
    // The entry may already be gone. Track this locally rather than reading
    // its prefs while draining an unfinished first write.
    file_.AsyncCall(base::IgnoreResult(&ImageFile::Remove));
  }
}

bool ProfileImage::HasSavedImage() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return entry().GetBool(kSaved);
}

bool ProfileImage::IsActive() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return IsEnabled() && HasSavedImage() && entry().GetBool(kActive);
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
  // The file sequence gets its own immutable copy. Copying fails if the pixels
  // cannot be read.
  SkBitmap copy;
  if (!copy.tryAllocPixels(bitmap.info()) ||
      !bitmap.readPixels(copy.pixmap(), 0, 0)) {
    ReplyLater(std::move(callback), Result::kInvalidImage);
    return;
  }
  copy.setImmutable();
  const auto generation = generation_;
  file_.AsyncCall(&ImageFile::Replace)
      .WithArgs(copy)
      .Then(base::BindOnce(&ProfileImage::Replaced, weak_factory_.GetWeakPtr(),
                           generation, gfx::Image::CreateFrom1xBitmap(copy),
                           std::move(callback)));
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
  entry().SetBool(kSaved, true);
  if (generation == generation_) {
    entry().SetBool(kActive, true);
  }
  Notify();
  std::move(callback).Run(Result::kSuccess);
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
  entry().SetBool(kSaved, false);
  entry().SetBool(kActive, false);
  Notify();
  std::move(callback).Run(Result::kSuccess);
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
    entry().SetBool(kActive, true);
    Notify();
  }
  std::move(callback).Run(Result::kSuccess);
}

void ProfileImage::SelectFallback() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Overrides a pending Replace or Select even before anything is saved.
  ++generation_;
  // Record preset choices made while the feature is off so re-enabling it
  // does not restore the custom image over the user's later choice. Without a
  // saved image the flag means nothing, so Local State is left alone.
  if (HasSavedImage() && entry().SetBool(kActive, false)) {
    Notify();
  }
}

ProfileAttributesEntry& ProfileImage::entry() const {
  ProfileAttributesEntry* entry = storage_->GetProfileAttributesWithPath(path_);
  CHECK(entry);
  return *entry;
}

void ProfileImage::Notify() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  storage_->NotifyOnProfileAvatarChanged(path_);
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

void BraveRemoveCustomProfileImage(ProfileAttributesStorage& storage,
                                   const base::FilePath& profile_path) {
  custom_profile_image::ProfileImage::ProfileRemoved(storage, profile_path);
}

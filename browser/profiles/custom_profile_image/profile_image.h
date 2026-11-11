// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PROFILES_CUSTOM_PROFILE_IMAGE_PROFILE_IMAGE_H_
#define BRAVE_BROWSER_PROFILES_CUSTOM_PROFILE_IMAGE_PROFILE_IMAGE_H_

#include <cstdint>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/supports_user_data.h"
#include "base/task/sequenced_task_runner.h"
#include "base/threading/sequence_bound.h"
#include "brave/browser/profiles/custom_profile_image/image_file.h"
#include "ui/gfx/image/image.h"

class ProfileAttributesEntry;
class ProfileAttributesStorage;

namespace custom_profile_image {

enum class Result { kSuccess, kInvalidImage, kFileError, kUnavailable };

// ProfileAttributesEntry owns this object. All callers share its file sequence
// and selection generation. Callbacks always run asynchronously.
class ProfileImage : public base::SupportsUserData::Data {
 public:
  using Callback = base::OnceCallback<void(Result)>;
  static ProfileImage& Get(ProfileAttributesEntry& entry);
  static bool IsEnabled();

  ~ProfileImage() override;
  bool HasSavedImage() const;
  bool IsActive() const;
  // Active and not known to be unreadable. Stays true while the image loads so
  // the stored preset is not shown as selected in the meantime.
  bool IsSelected() const;
  const gfx::Image& GetImage();
  // Saves `bitmap` (square, at most kMaxImageSide px) and makes it the
  // profile's selection. `callback` receives kInvalidImage for an unusable
  // bitmap, kFileError if the write fails (the previous image stays) and
  // kUnavailable when the feature is off or the profile was removed. A preset
  // chosen while the write is pending wins: the image is saved but not
  // selected.
  void Replace(const SkBitmap& bitmap, Callback callback);
  // Deletes the saved image and clears the selection. Returns kFileError if
  // the file cannot be deleted (the image stays), and kUnavailable when the
  // feature is off or the profile was removed.
  void Remove(Callback callback);
  // Re-reads the saved file and selects it again. Returns kFileError if the
  // file is missing or damaged, and kUnavailable when nothing is saved, the
  // feature is off or the profile was removed.
  void Select(Callback callback);
  // Records that a preset was chosen: deactivates the custom image and
  // overrides any pending Replace or Select.
  void SelectFallback();
  // Deletes the file after already accepted writes. Callbacks still waiting
  // for the file sequence never run; results already posted still do.
  void ProfileRemoved();

 private:
  explicit ProfileImage(ProfileAttributesEntry& entry);
  static scoped_refptr<base::SequencedTaskRunner> FileRunner(
      ProfileAttributesStorage& storage);
  void Loaded(SkBitmap bitmap);
  void Replaced(uint64_t generation,
                gfx::Image image,
                Callback callback,
                bool success);
  void Removed(Callback callback, bool success);
  void Selected(uint64_t generation, Callback callback, SkBitmap bitmap);
  void Notify();

  base::SequenceBound<ImageFile> file_;
  gfx::Image image_;
  bool loading_ = false;
  bool loaded_ = false;
  bool removed_ = false;
  bool load_failed_ = false;
  bool has_committed_image_;
  // File replies are ordered. Only an immediate fallback choice can overtake
  // them; a later failed file operation must not cancel an earlier selection.
  uint64_t generation_ = 0;
  const raw_ref<ProfileAttributesEntry> entry_;
  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<ProfileImage> weak_factory_{this};
};

}  // namespace custom_profile_image

#endif  // BRAVE_BROWSER_PROFILES_CUSTOM_PROFILE_IMAGE_PROFILE_IMAGE_H_

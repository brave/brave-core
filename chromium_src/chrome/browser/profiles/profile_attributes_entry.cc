/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile_attributes_entry.h"

#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "chrome/browser/profiles/brave_profile_attributes_constants.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"

namespace {
constexpr std::string_view kSerpMetricsKey = "serp_metrics";

// Key under the entry dict where the on-disk filename of the user-uploaded
// custom profile avatar is stored. When empty, no custom avatar is set.
constexpr char kBraveCustomAvatarFileNameKey[] =
    "brave_custom_avatar_file_name";

// When true (and a custom file exists), the profile icon uses the custom image.
// Absent key with an existing file is treated as true for backward
// compatibility.
constexpr char kBraveCustomAvatarActiveKey[] = "brave_custom_avatar_active";

// Returns a cache key (distinct from the GAIA cache key) for use with
// `ProfileAttributesStorage::LoadAvatarPictureFromPath` so the custom avatar
// image does not collide with the cached GAIA image.
std::string BraveCustomAvatarCacheKey(const std::string& storage_key) {
  return storage_key + "_brave_custom_avatar";
}
}  // namespace

void ProfileAttributesEntry::BraveMigrateObsoleteProfileAttributes() {
  // Run our migrations
#if !BUILDFLAG(IS_ANDROID)
  const int kPlaceholderAvatarIndex =
      static_cast<int>(profiles::GetPlaceholderAvatarIndex());

  const int kBraveAvatarIconStartIndex =
      static_cast<int>(profiles::GetDefaultAvatarIconCount() -
                       profiles::kBraveDefaultAvatarIconsCount);
  // Added 25 July 2024
  // see https://github.com/brave/brave-browser/issues/40005
  //
  // Brave originally allowed folks to pick the Chromium profile icons.
  // We then removed those in favor of our own branded icons in 0.70.x (2019).
  // https://github.com/brave/brave-core/pull/3165
  //
  // The old ones would continue to work - but may have had rendering issues.
  // Chromium 127 had a Windows change which now triggers a CHECK.
  //
  // This migration moves folks who have the old IDs to the default profile ID.
  int icon_index = GetAvatarIconIndex();
  if (icon_index < kBraveAvatarIconStartIndex &&
      icon_index != kPlaceholderAvatarIndex) {
    SetAvatarIconIndex(kPlaceholderAvatarIndex);
  }
#endif
}

// Retrieves a SERP metric from profile attributes.
const base::DictValue* ProfileAttributesEntry::GetSerpMetrics() const {
  const base::Value* serp_metrics = GetValue(kSerpMetricsKey.data());
  if (!serp_metrics) {
    return nullptr;
  }
  return serp_metrics->GetIfDict();
}

// Stores a SERP metric in profile attributes.
void ProfileAttributesEntry::SetSerpMetrics(base::DictValue serp_metrics) {
  SetValue(kSerpMetricsKey.data(), base::Value(std::move(serp_metrics)));
}

bool ProfileAttributesEntry::HasBraveCustomAvatar() const {
  return !GetString(kBraveCustomAvatarFileNameKey).empty();
}

bool ProfileAttributesEntry::IsUsingBraveCustomAvatar() const {
  if (!HasBraveCustomAvatar()) {
    return false;
  }
  const base::Value* active = GetValue(kBraveCustomAvatarActiveKey);
  if (!active || !active->is_bool()) {
    // Legacy profiles: file present before this pref existed → custom in use.
    return true;
  }
  return active->GetBool();
}

const gfx::Image* ProfileAttributesEntry::GetBraveCustomAvatar() const {
  const std::string file_name = GetString(kBraveCustomAvatarFileNameKey);
  if (file_name.empty()) {
    return nullptr;
  }
  const base::FilePath image_path = profile_path_.AppendASCII(file_name);
  return profile_attributes_storage_->LoadAvatarPictureFromPath(
      profile_path_, BraveCustomAvatarCacheKey(storage_key_), image_path);
}

void ProfileAttributesEntry::SetBraveCustomAvatar(
    gfx::Image image,
    base::OnceCallback<void(bool)> on_saved) {
  if (image.IsEmpty()) {
    ClearBraveCustomAvatar();
    if (on_saved) {
      std::move(on_saved).Run(false);
    }
    return;
  }

  // Persist the filename in the entry so subsequent `GetBraveCustomAvatar()`
  // calls find the cached image immediately (the storage cache is populated
  // synchronously by `SaveGAIAImageAtPath` below).
  SetString(kBraveCustomAvatarFileNameKey, brave::kBraveCustomAvatarFileName);
  SetBool(kBraveCustomAvatarActiveKey, true);

  const base::FilePath image_path =
      profile_path_.AppendASCII(brave::kBraveCustomAvatarFileName);
  // Reuse `SaveGAIAImageAtPath` for its PNG encode + background-write
  // pipeline. The lower-level `SaveAvatarImageAtPath` primitive is private
  // on `ProfileAttributesStorage` and `ProfileAttributesEntry` is not a
  // friend, so this is the only public entry point that gives us the
  // synchronous in-memory cache insert + background disk write semantics
  // we need. The only side effects outside of writing the file are
  // populating `cached_avatar_images_[key]` synchronously (so subsequent
  // `GetBraveCustomAvatar()` calls return the new image immediately even
  // before the file write completes) and re-setting the GAIA URL pref via
  // `OnGAIAPictureSaved`. We pass the current GAIA URL back to make the
  // latter a no-op for that pref.
  profile_attributes_storage_->SaveGAIAImageAtPath(
      profile_path_, BraveCustomAvatarCacheKey(storage_key_), std::move(image),
      image_path, GetLastDownloadedGAIAPictureUrlWithSize());

  // The image is now in the in-memory cache and the disk write has been
  // scheduled. Notify observers immediately so the profile menu, toolbar
  // button, and other `GetAvatarIcon` consumers repaint without waiting for
  // the background PNG write (which only fires `OnProfileHighResAvatarLoaded`,
  // not `OnProfileAvatarChanged`).
  profile_attributes_storage_->NotifyOnProfileAvatarChanged(profile_path_);

  if (on_saved) {
    std::move(on_saved).Run(true);
  }
}

void ProfileAttributesEntry::ClearBraveCustomAvatar() {
  const std::string file_name = GetString(kBraveCustomAvatarFileNameKey);
  if (file_name.empty()) {
    return;
  }

  // Clear the entry pointer before `NotifyOnProfileAvatarChanged` so any
  // synchronous observers see the cleared state when the notification fires.
  SetString(kBraveCustomAvatarFileNameKey, std::string());
  ClearValue(kBraveCustomAvatarActiveKey);

  // Delete the file from disk on a background thread. We deliberately don't
  // use `DeleteGAIAImageAtPath` because it has the side effect of clearing
  // the GAIA URL pref. The storage cache entry under the custom-avatar cache
  // key is never queried again unless the user uploads a new custom image,
  // at which point `SaveGAIAImageAtPath` invalidates it.
  const base::FilePath image_path = profile_path_.AppendASCII(file_name);
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(base::IgnoreResult(&base::DeleteFile), image_path));

  profile_attributes_storage_->NotifyOnProfileAvatarChanged(profile_path_);
}

void ProfileAttributesEntry::DeactivateBraveCustomAvatar() {
  if (!HasBraveCustomAvatar()) {
    return;
  }
  if (!IsUsingBraveCustomAvatar()) {
    return;
  }
  SetBool(kBraveCustomAvatarActiveKey, false);
  profile_attributes_storage_->NotifyOnProfileAvatarChanged(profile_path_);
}

void ProfileAttributesEntry::ActivateBraveCustomAvatar() {
  if (!HasBraveCustomAvatar()) {
    return;
  }
  if (IsUsingBraveCustomAvatar()) {
    return;
  }
  SetBool(kBraveCustomAvatarActiveKey, true);
  profile_attributes_storage_->NotifyOnProfileAvatarChanged(profile_path_);
}

#define BRAVE_PROFILE_ATTRIBUTES_ENTRY_MIGRATE_OBSOLETE_PROFILE_ATTRIBUTES \
  BraveMigrateObsoleteProfileAttributes();

// When a user-uploaded custom profile avatar is present, return it before any
// of the upstream GAIA / default / theme resolution paths run, so that every
// caller of `GetAvatarIcon` / `GetAvatarIconWithType` (toolbar button, profile
// menu, profile picker, OS menus, settings, etc.) consistently displays the
// custom image.
//
// `GetBraveCustomAvatar()` may return a non-null but still-empty `gfx::Image`
// on the very first access after a restart while the bitmap is being loaded
// from disk. In that case fall through to the upstream resolution so the UI
// keeps showing the previous (GAIA / preset / placeholder) avatar instead of
// flashing an empty icon; the storage's `OnProfileHighResAvatarLoaded`
// notification will trigger a repaint once the bitmap is ready.
#define BRAVE_GET_AVATAR_ICON_WITH_TYPE                   \
  if (IsUsingBraveCustomAvatar()) {                       \
    if (const gfx::Image* image = GetBraveCustomAvatar(); \
        image && !image->IsEmpty()) {                     \
      return {*image, AvatarIconType::kNonPlaceholder};   \
    }                                                     \
  }

#include <chrome/browser/profiles/profile_attributes_entry.cc>
#undef BRAVE_GET_AVATAR_ICON_WITH_TYPE
#undef BRAVE_PROFILE_ATTRIBUTES_ENTRY_MIGRATE_OBSOLETE_PROFILE_ATTRIBUTES

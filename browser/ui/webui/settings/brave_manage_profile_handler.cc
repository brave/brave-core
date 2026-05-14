/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_manage_profile_handler.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "content/public/browser/browser_thread.h"
#include "skia/ext/image_operations.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/gfx/image/image.h"

namespace {

ProfileAttributesEntry* GetEntry(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  return g_browser_process->profile_manager()
      ->GetProfileAttributesStorage()
      .GetProfileAttributesWithPath(profile->GetPath());
}

// Center-square-crops `bitmap` then resizes the crop to `side x side`.
SkBitmap CropAndResizeToSquare(const SkBitmap& bitmap, int side) {
  CHECK_GT(side, 0);
  if (bitmap.drawsNothing()) {
    return SkBitmap();
  }

  const int crop_side = std::min(bitmap.width(), bitmap.height());
  const int crop_x = (bitmap.width() - crop_side) / 2;
  const int crop_y = (bitmap.height() - crop_side) / 2;

  SkBitmap cropped;
  if (!bitmap.extractSubset(
          &cropped, SkIRect::MakeXYWH(crop_x, crop_y, crop_side, crop_side))) {
    return SkBitmap();
  }

  return skia::ImageOperations::Resize(
      cropped, skia::ImageOperations::RESIZE_BEST, side, side);
}

}  // namespace

BraveManageProfileHandler::BraveManageProfileHandler(Profile* profile)
    : profile_(profile) {
  CHECK(profile_);
  storage_observation_.Observe(
      &g_browser_process->profile_manager()->GetProfileAttributesStorage());
  profile_observation_.Observe(profile_.get());
}

BraveManageProfileHandler::~BraveManageProfileHandler() {
  ImageDecoder::Cancel(this);
}

void BraveManageProfileHandler::BindUI(
    mojo::PendingRemote<
        brave_manage_profile::mojom::BraveManageProfileSettingsUI> ui) {
  // The settings page binds at most once per handler lifetime; resetting
  // covers the in-test case where a fresh remote replaces a stale one.
  ui_.reset();
  ui_.Bind(std::move(ui));
}

void BraveManageProfileHandler::GetCustomAvatar(
    GetCustomAvatarCallback callback) {
  std::move(callback).Run(BuildCustomAvatarState());
}

void BraveManageProfileHandler::SetCustomAvatar(
    const std::vector<uint8_t>& bytes,
    SetCustomAvatarCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  using brave_manage_profile::mojom::SetCustomAvatarError;

  if (!profile_) {
    std::move(callback).Run(SetCustomAvatarError::kProfileShuttingDown,
                            BuildCustomAvatarState());
    return;
  }

  if (bytes.empty()) {
    std::move(callback).Run(SetCustomAvatarError::kEmpty,
                            BuildCustomAvatarState());
    return;
  }
  if (bytes.size() > kMaxUploadBytes) {
    std::move(callback).Run(SetCustomAvatarError::kTooLarge,
                            BuildCustomAvatarState());
    return;
  }

  // Cancel any in-flight decode for an earlier upload before starting a new
  // one — only one upload result can be honored at a time.
  if (pending_upload_callback_) {
    ImageDecoder::Cancel(this);
    std::move(pending_upload_callback_)
        .Run(SetCustomAvatarError::kSuperseded, BuildCustomAvatarState());
  }
  pending_upload_callback_ = std::move(callback);

  ImageDecoder::Start(this, bytes);
}

void BraveManageProfileHandler::RemoveCustomAvatar() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!profile_) {
    return;
  }
  ProfileAttributesEntry* entry = GetEntry(profile_);
  if (!entry) {
    return;
  }
  entry->ClearBraveCustomAvatar();
  // `ClearBraveCustomAvatar` fires `OnProfileAvatarChanged` which will trigger
  // `NotifyCustomAvatarChanged` through our observer; no extra signal needed.
}

void BraveManageProfileHandler::ActivateCustomAvatar() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!profile_) {
    return;
  }
  ProfileAttributesEntry* entry = GetEntry(profile_);
  if (!entry) {
    return;
  }
  entry->ActivateBraveCustomAvatar();
}

void BraveManageProfileHandler::OnImageDecoded(const SkBitmap& decoded_image) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  using brave_manage_profile::mojom::SetCustomAvatarError;

  SetCustomAvatarCallback callback = std::move(pending_upload_callback_);

  // `OnProfileWillBeDestroyed` already drains `pending_upload_callback_`, so
  // a missing `profile_` here can only happen for an in-flight decode that
  // raced past the teardown notification. Surface it cleanly either way.
  if (!profile_) {
    if (callback) {
      std::move(callback).Run(SetCustomAvatarError::kProfileShuttingDown,
                              BuildCustomAvatarState());
    }
    return;
  }

  const SkBitmap normalized = CropAndResizeToSquare(decoded_image, kAvatarSize);
  if (normalized.drawsNothing()) {
    if (callback) {
      std::move(callback).Run(SetCustomAvatarError::kDecodeFailed,
                              BuildCustomAvatarState());
    }
    return;
  }

  ProfileAttributesEntry* entry = GetEntry(profile_);
  if (!entry) {
    if (callback) {
      std::move(callback).Run(SetCustomAvatarError::kNoProfileEntry,
                              BuildCustomAvatarState());
    }
    return;
  }

  gfx::Image image = gfx::Image::CreateFrom1xBitmap(normalized);
  entry->SetBraveCustomAvatar(
      std::move(image),
      base::BindOnce(
          [](base::WeakPtr<BraveManageProfileHandler> self,
             SetCustomAvatarCallback callback, bool success) {
            if (!self || !callback) {
              return;
            }
            if (success) {
              std::move(callback).Run(std::nullopt,
                                      self->BuildCustomAvatarState());
            } else {
              std::move(callback).Run(SetCustomAvatarError::kSaveFailed,
                                      self->BuildCustomAvatarState());
            }
          },
          weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveManageProfileHandler::OnDecodeImageFailed() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  using brave_manage_profile::mojom::SetCustomAvatarError;

  if (pending_upload_callback_) {
    std::move(pending_upload_callback_)
        .Run(SetCustomAvatarError::kDecodeFailed, BuildCustomAvatarState());
  }
}

void BraveManageProfileHandler::OnProfileAvatarChanged(
    const base::FilePath& profile_path) {
  if (profile_path != profile_->GetPath()) {
    return;
  }
  NotifyCustomAvatarChanged();
}

void BraveManageProfileHandler::OnProfileHighResAvatarLoaded(
    const base::FilePath& profile_path) {
  if (profile_path != profile_->GetPath()) {
    return;
  }
  // The custom avatar bitmap may have just finished loading from disk after a
  // restart; rebuild the state so the front-end gets the now-decoded preview.
  NotifyCustomAvatarChanged();
}

void BraveManageProfileHandler::OnProfileWillBeDestroyed(Profile* profile) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  CHECK_EQ(profile, profile_.get());

  // Cancel any in-flight decode and resolve its pending callback so the
  // Mojo response is not silently dropped.
  ImageDecoder::Cancel(this);
  if (pending_upload_callback_) {
    std::move(pending_upload_callback_)
        .Run(brave_manage_profile::mojom::SetCustomAvatarError::
                 kProfileShuttingDown,
             BuildCustomAvatarState());
  }

  // Stop observing before clearing `profile_` so the scoped observations
  // don't outlive their target.
  profile_observation_.Reset();
  storage_observation_.Reset();

  // Tear down the push channel; no more state updates are possible.
  ui_.reset();

  profile_ = nullptr;
}

brave_manage_profile::mojom::CustomAvatarStatePtr
BraveManageProfileHandler::BuildCustomAvatarState() const {
  auto state = brave_manage_profile::mojom::CustomAvatarState::New();
  ProfileAttributesEntry* entry = GetEntry(profile_);
  state->has_saved_avatar = entry && entry->HasBraveCustomAvatar();
  state->is_active = entry && entry->IsUsingBraveCustomAvatar();
  if (state->has_saved_avatar) {
    const gfx::Image* image = entry->GetBraveCustomAvatar();
    // The image may still be loading from disk on the very first access after
    // a restart; in that case `data_url` stays empty and the storage will
    // re-notify via `OnProfileAvatarChanged` once loaded.
    if (image && !image->IsEmpty()) {
      state->data_url = webui::GetBitmapDataUrl(image->AsBitmap());
    }
  }
  return state;
}

void BraveManageProfileHandler::NotifyCustomAvatarChanged() {
  if (ui_) {
    ui_->OnCustomAvatarChanged(BuildCustomAvatarState());
  }
}

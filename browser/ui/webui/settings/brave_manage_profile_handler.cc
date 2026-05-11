/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_manage_profile_handler.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_ui.h"
#include "skia/ext/image_operations.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"

namespace {

constexpr char kMessageGetCustomAvatar[] = "getProfileCustomAvatar";
constexpr char kMessageSetCustomAvatar[] = "setProfileCustomAvatar";
constexpr char kMessageRemoveCustomAvatar[] = "removeProfileCustomAvatar";
constexpr char kMessageActivateCustomAvatar[] = "activateProfileCustomAvatar";

constexpr char kListenerCustomAvatarChanged[] = "brave-custom-avatar-changed";

ProfileAttributesEntry* GetEntry(Profile* profile) {
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
  if (!bitmap.extractSubset(&cropped,
                            SkIRect::MakeXYWH(crop_x, crop_y, crop_side,
                                              crop_side))) {
    return SkBitmap();
  }

  return skia::ImageOperations::Resize(
      cropped, skia::ImageOperations::RESIZE_BEST, side, side);
}

}  // namespace

BraveManageProfileHandler::BraveManageProfileHandler(Profile* profile)
    : profile_(profile) {
  CHECK(profile_);
}

BraveManageProfileHandler::~BraveManageProfileHandler() {
  ImageDecoder::Cancel(this);
}

void BraveManageProfileHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      kMessageGetCustomAvatar,
      base::BindRepeating(
          &BraveManageProfileHandler::HandleGetProfileCustomAvatar,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kMessageSetCustomAvatar,
      base::BindRepeating(
          &BraveManageProfileHandler::HandleSetProfileCustomAvatar,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kMessageRemoveCustomAvatar,
      base::BindRepeating(
          &BraveManageProfileHandler::HandleRemoveProfileCustomAvatar,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kMessageActivateCustomAvatar,
      base::BindRepeating(
          &BraveManageProfileHandler::HandleActivateProfileCustomAvatar,
          base::Unretained(this)));
}

void BraveManageProfileHandler::OnJavascriptAllowed() {
  storage_observation_.Observe(
      &g_browser_process->profile_manager()->GetProfileAttributesStorage());
}

void BraveManageProfileHandler::OnJavascriptDisallowed() {
  storage_observation_.Reset();
  ImageDecoder::Cancel(this);
  pending_upload_callback_id_.clear();
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void BraveManageProfileHandler::OnProfileAvatarChanged(
    const base::FilePath& profile_path) {
  if (profile_path != profile_->GetPath() || !IsJavascriptAllowed()) {
    return;
  }
  FireCustomAvatarChanged();
}

void BraveManageProfileHandler::OnProfileHighResAvatarLoaded(
    const base::FilePath& profile_path) {
  if (profile_path != profile_->GetPath() || !IsJavascriptAllowed()) {
    return;
  }
  // The custom avatar bitmap may have just finished loading from disk after a
  // restart; rebuild the state so the front-end gets the now-decoded preview.
  FireCustomAvatarChanged();
}

void BraveManageProfileHandler::HandleGetProfileCustomAvatar(
    const base::ListValue& args) {
  AllowJavascript();
  CHECK_EQ(1u, args.size());
  const base::Value& callback_id = args[0];
  ResolveJavascriptCallback(callback_id, BuildCustomAvatarState());
}

void BraveManageProfileHandler::HandleSetProfileCustomAvatar(
    const base::ListValue& args) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  AllowJavascript();

  CHECK_EQ(2u, args.size());
  const std::string callback_id = args[0].GetString();
  if (!args[1].is_string()) {
    RejectJavascriptCallback(base::Value(callback_id),
                             base::Value("invalid-payload"));
    return;
  }
  const std::string& base64_payload = args[1].GetString();
  if (base64_payload.empty()) {
    RejectJavascriptCallback(base::Value(callback_id), base::Value("empty"));
    return;
  }
  // Rough cap on the encoded payload size before decoding (base64 grows by
  // ~4/3, so this comfortably bounds the decoded size below `kMaxUploadBytes`).
  if (base64_payload.size() > (kMaxUploadBytes * 4 / 3) + 4) {
    RejectJavascriptCallback(base::Value(callback_id),
                             base::Value("too-large"));
    return;
  }

  std::string raw_bytes;
  if (!base::Base64Decode(base64_payload, &raw_bytes) || raw_bytes.empty()) {
    RejectJavascriptCallback(base::Value(callback_id),
                             base::Value("invalid-base64"));
    return;
  }
  if (raw_bytes.size() > kMaxUploadBytes) {
    RejectJavascriptCallback(base::Value(callback_id),
                             base::Value("too-large"));
    return;
  }

  // Cancel any in-flight decode for an earlier upload before starting a new
  // one - only one upload result can be honored at a time.
  if (!pending_upload_callback_id_.empty()) {
    ImageDecoder::Cancel(this);
    RejectJavascriptCallback(base::Value(pending_upload_callback_id_),
                             base::Value("superseded"));
    pending_upload_callback_id_.clear();
  }
  pending_upload_callback_id_ = callback_id;

  std::vector<uint8_t> image_data(raw_bytes.begin(), raw_bytes.end());
  ImageDecoder::Start(this, std::move(image_data));
}

void BraveManageProfileHandler::HandleRemoveProfileCustomAvatar(
    const base::ListValue& /*args*/) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  AllowJavascript();

  ProfileAttributesEntry* entry = GetEntry(profile_);
  if (!entry) {
    return;
  }
  entry->ClearBraveCustomAvatar();
  // `ClearBraveCustomAvatar` fires `OnProfileAvatarChanged` which will trigger
  // `FireCustomAvatarChanged` through our observer; no extra signal needed.
}

void BraveManageProfileHandler::HandleActivateProfileCustomAvatar(
    const base::ListValue& /*args*/) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  AllowJavascript();

  ProfileAttributesEntry* entry = GetEntry(profile_);
  if (!entry) {
    return;
  }
  entry->ActivateBraveCustomAvatar();
}

void BraveManageProfileHandler::OnImageDecoded(const SkBitmap& decoded_image) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::string callback_id;
  callback_id.swap(pending_upload_callback_id_);

  const SkBitmap normalized = CropAndResizeToSquare(decoded_image, kAvatarSize);
  if (normalized.drawsNothing()) {
    if (IsJavascriptAllowed() && !callback_id.empty()) {
      RejectJavascriptCallback(base::Value(callback_id),
                               base::Value("decode-failed"));
    }
    return;
  }

  ProfileAttributesEntry* entry = GetEntry(profile_);
  if (!entry) {
    if (IsJavascriptAllowed() && !callback_id.empty()) {
      RejectJavascriptCallback(base::Value(callback_id),
                               base::Value("no-profile-entry"));
    }
    return;
  }

  gfx::Image image = gfx::Image::CreateFrom1xBitmap(normalized);
  entry->SetBraveCustomAvatar(
      std::move(image),
      base::BindOnce(
          [](base::WeakPtr<BraveManageProfileHandler> self,
             std::string callback_id, bool success) {
            if (!self || !self->IsJavascriptAllowed() || callback_id.empty()) {
              return;
            }
            if (success) {
              self->ResolveJavascriptCallback(base::Value(callback_id),
                                              self->BuildCustomAvatarState());
            } else {
              self->RejectJavascriptCallback(base::Value(callback_id),
                                             base::Value("save-failed"));
            }
          },
          weak_ptr_factory_.GetWeakPtr(), std::move(callback_id)));
}

void BraveManageProfileHandler::OnDecodeImageFailed() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::string callback_id;
  callback_id.swap(pending_upload_callback_id_);
  if (IsJavascriptAllowed() && !callback_id.empty()) {
    RejectJavascriptCallback(base::Value(callback_id),
                             base::Value("decode-failed"));
  }
}

base::DictValue BraveManageProfileHandler::BuildCustomAvatarState() const {
  base::DictValue state;
  ProfileAttributesEntry* entry = GetEntry(profile_);
  const bool has_saved = entry && entry->HasBraveCustomAvatar();
  const bool is_active = entry && entry->IsUsingBraveCustomAvatar();
  state.Set("hasSavedAvatar", has_saved);
  state.Set("isActive", is_active);
  if (has_saved) {
    const gfx::Image* image = entry->GetBraveCustomAvatar();
    // The image may still be loading from disk on the very first access after
    // a restart; in that case `dataUrl` stays empty and the storage will
    // re-notify via `OnProfileAvatarChanged` once loaded.
    if (image && !image->IsEmpty()) {
      state.Set("dataUrl", webui::GetBitmapDataUrl(image->AsBitmap()));
    }
  }
  return state;
}

void BraveManageProfileHandler::FireCustomAvatarChanged() {
  if (!IsJavascriptAllowed()) {
    return;
  }
  FireWebUIListener(kListenerCustomAvatarChanged, BuildCustomAvatarState());
}

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_MANAGE_PROFILE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_MANAGE_PROFILE_HANDLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "chrome/browser/image_decoder/image_decoder.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class Profile;

namespace base {
class DictValue;
class ListValue;
}  // namespace base

// Backs `brave://settings/manageProfile`'s "Upload your own image" row.
//
// Receives the raw bytes of a user-selected image, decodes them in a
// sandboxed image_decoder service, normalizes the result (square-crop +
// resize to `kAvatarSize`), persists it as a PNG via the entry's
// `SetBraveCustomAvatar` API, and notifies the WebUI so the page can show
// the new preview.
class BraveManageProfileHandler
    : public settings::SettingsPageUIHandler,
      public ImageDecoder::ImageRequest,
      public ProfileAttributesStorage::Observer {
 public:
  // Side length (in pixels) of the persisted custom avatar bitmap.
  static constexpr int kAvatarSize = 256;
  // Maximum size of the upload payload in bytes (rejected before decode).
  static constexpr size_t kMaxUploadBytes = 10 * 1024 * 1024;

  explicit BraveManageProfileHandler(Profile* profile);
  BraveManageProfileHandler(const BraveManageProfileHandler&) = delete;
  BraveManageProfileHandler& operator=(const BraveManageProfileHandler&) =
      delete;
  ~BraveManageProfileHandler() override;

 private:
  // SettingsPageUIHandler:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  // ImageDecoder::ImageRequest:
  void OnImageDecoded(const SkBitmap& decoded_image) override;
  void OnDecodeImageFailed() override;

  // ProfileAttributesStorage::Observer:
  void OnProfileAvatarChanged(const base::FilePath& profile_path) override;
  void OnProfileHighResAvatarLoaded(
      const base::FilePath& profile_path) override;

  // WebUI messages.
  void HandleSetProfileCustomAvatar(const base::ListValue& args);
  void HandleRemoveProfileCustomAvatar(const base::ListValue& args);
  void HandleGetProfileCustomAvatar(const base::ListValue& args);

  // Builds the `{hasAvatar, dataUrl}` dictionary that the front-end consumes.
  base::DictValue BuildCustomAvatarState() const;

  // Fires `brave-custom-avatar-changed` with the latest state.
  void FireCustomAvatarChanged();

  const raw_ptr<Profile> profile_;

  // Pending callback id for an in-flight upload (only one upload may be in
  // flight at a time; subsequent uploads cancel any earlier decode).
  std::string pending_upload_callback_id_;

  base::ScopedObservation<ProfileAttributesStorage,
                          ProfileAttributesStorage::Observer>
      storage_observation_{this};

  base::WeakPtrFactory<BraveManageProfileHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_MANAGE_PROFILE_HANDLER_H_

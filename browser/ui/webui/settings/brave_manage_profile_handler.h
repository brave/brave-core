/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_MANAGE_PROFILE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_MANAGE_PROFILE_HANDLER_H_

#include <cstdint>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/webui/settings/brave_manage_profile.mojom.h"
#include "chrome/browser/image_decoder/image_decoder.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_observer.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

class Profile;

// Backs `brave://settings/manageProfile`'s "Upload your own image" row over a
// Mojo interface.
//
// Receives the raw bytes of a user-selected image, decodes them in a
// sandboxed image_decoder service, normalizes the result (square-crop +
// resize to `kAvatarSize`), persists it as a PNG via the entry's
// `SetBraveCustomAvatar` API, and pushes state updates to the WebUI through
// the bound `BraveManageProfileSettingsUI` remote.
class BraveManageProfileHandler
    : public brave_manage_profile::mojom::BraveManageProfileSettingsHandler,
      public ImageDecoder::ImageRequest,
      public ProfileAttributesStorage::Observer,
      public ProfileObserver {
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

  // brave_manage_profile::mojom::BraveManageProfileSettingsHandler:
  void BindUI(mojo::PendingRemote<
              brave_manage_profile::mojom::BraveManageProfileSettingsUI> ui)
      override;
  void GetCustomAvatar(GetCustomAvatarCallback callback) override;
  void SetCustomAvatar(const std::vector<uint8_t>& bytes,
                       SetCustomAvatarCallback callback) override;
  void RemoveCustomAvatar() override;
  void ActivateCustomAvatar() override;

 private:
  // ImageDecoder::ImageRequest:
  void OnImageDecoded(const SkBitmap& decoded_image) override;
  void OnDecodeImageFailed() override;

  // ProfileAttributesStorage::Observer:
  void OnProfileAvatarChanged(const base::FilePath& profile_path) override;
  void OnProfileHighResAvatarLoaded(
      const base::FilePath& profile_path) override;

  // ProfileObserver:
  void OnProfileWillBeDestroyed(Profile* profile) override;

  // Builds the custom-avatar state struct for the front-end.
  brave_manage_profile::mojom::CustomAvatarStatePtr BuildCustomAvatarState()
      const;

  // Pushes the latest state to the bound `BraveManageProfileSettingsUI`, if
  // any.
  void NotifyCustomAvatarChanged();

  // Push channel from the browser to the settings page. Bound once via
  // `BindUI`; left unbound until then so observer events become a no-op.
  mojo::Remote<brave_manage_profile::mojom::BraveManageProfileSettingsUI> ui_;

  // Pending response for an in-flight upload (only one upload may be in
  // flight at a time; a subsequent upload cancels the earlier decode and
  // rejects its pending callback with `kSuperseded`).
  SetCustomAvatarCallback pending_upload_callback_;

  base::ScopedObservation<ProfileAttributesStorage,
                          ProfileAttributesStorage::Observer>
      storage_observation_{this};

  // Watches for the owning profile's destruction so synchronous methods can
  // null-check `profile_` and stop dereferencing a destroyed profile.
  base::ScopedObservation<Profile, ProfileObserver> profile_observation_{this};

  // Cleared in `OnProfileWillBeDestroyed`. All entry points must null-check
  // before dereferencing.
  raw_ptr<Profile> profile_;

  base::WeakPtrFactory<BraveManageProfileHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_MANAGE_PROFILE_HANDLER_H_

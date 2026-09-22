/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_WALLPAPER_PROVIDER_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_WALLPAPER_PROVIDER_H_

#include <optional>

#include "base/functional/callback.h"
#include "base/values.h"

namespace ntp_background_images {

// Each NTP background type owns its own eligibility and payload logic in one
// class, so adding a new type is one class and one registration, not a change
// to shared conditional logic.
class NTPWallpaperProvider {
 public:
  using MaybeGetWallpaperCallback =
      base::OnceCallback<void(std::optional<base::DictValue>)>;

  virtual ~NTPWallpaperProvider() = default;

  virtual bool IsEligible() const = 0;

  // The callback may still run with nullopt when `IsEligible` returned true,
  // for example when the underlying data turns out to be empty or an async
  // fetch fails. Whether to fall back to another provider in that case is up
  // to the caller; no current caller does so. Producing a wallpaper can
  // require async work, such as serving an ad, so this takes a callback
  // rather than returning a value directly.
  virtual void MaybeGetWallpaper(MaybeGetWallpaperCallback callback) = 0;
};

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_WALLPAPERS_NTP_WALLPAPER_PROVIDER_H_

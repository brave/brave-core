// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/brave_ntp_custom_background_service.h"

#include <utility>

#include "base/check.h"
#include "base/files/file_path.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "url/gurl.h"

namespace ntp_background_images {

BraveNTPCustomBackgroundService::BraveNTPCustomBackgroundService(
    std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)) {
  DCHECK(delegate_);
}

BraveNTPCustomBackgroundService::~BraveNTPCustomBackgroundService() = default;

bool BraveNTPCustomBackgroundService::ShouldShowCustomBackground() const {
  return delegate_->IsCustomImageBackgroundEnabled() ||
         delegate_->IsColorBackgroundEnabled() ||
         delegate_->HasPreferredBraveBackground();
}

base::DictValue BraveNTPCustomBackgroundService::GetBackground() const {
  DCHECK(ShouldShowCustomBackground());

  if (delegate_->HasPreferredBraveBackground()) {
    base::DictValue background = delegate_->GetPreferredBraveBackground();
    if (background.empty()) {
      // Return empty value so that it falls back to random Brave background.
      return background;
    }

    background.Set(kWallpaperRandomKey, false);
    return background;
  }

  // The |data| will be mapped to NewTab.BackgroundWallpaper type from JS side.
  // So we need to keep names of properties same.
  base::DictValue data;
  data.Set(kIsBackgroundKey, true);
  if (delegate_->IsCustomImageBackgroundEnabled()) {
    data.Set(kWallpaperURLKey, delegate_->GetCustomBackgroundImageURL().spec());
    data.Set(kWallpaperTypeKey, "image");
    data.Set(kWallpaperRandomKey, delegate_->ShouldUseRandomValue());
  } else if (delegate_->IsColorBackgroundEnabled()) {
    data.Set(kWallpaperColorKey, delegate_->GetColor());
    data.Set(kWallpaperTypeKey, "color");
    data.Set(kWallpaperRandomKey, delegate_->ShouldUseRandomValue());
  }
  return data;
}

base::FilePath BraveNTPCustomBackgroundService::GetImageFilePath(
    const GURL& url) {
  return delegate_->GetCustomBackgroundImageLocalFilePath(url);
}

void BraveNTPCustomBackgroundService::Shutdown() {
  delegate_.reset();
}

}  // namespace ntp_background_images

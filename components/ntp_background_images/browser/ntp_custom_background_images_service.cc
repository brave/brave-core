// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/ntp_custom_background_images_service.h"

#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/values.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"

namespace ntp_background_images {

NTPCustomBackgroundImagesService::NTPCustomBackgroundImagesService(
    std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)) {
  DCHECK(delegate_);
}

NTPCustomBackgroundImagesService::~NTPCustomBackgroundImagesService() = default;

bool NTPCustomBackgroundImagesService::ShouldShowCustomBackground() const {
  return delegate_->IsCustomImageBackgroundEnabled() ||
         delegate_->IsSolidColorBackgroundEnabled();
}

base::Value NTPCustomBackgroundImagesService::GetBackground() const {
  DCHECK(ShouldShowCustomBackground());

  base::Value data(base::Value::Type::DICTIONARY);
  data.SetBoolKey(kIsBackgroundKey, true);
  if (delegate_->IsCustomImageBackgroundEnabled()) {
    data.SetStringKey(kWallpaperImageURLKey, kCustomWallpaperURL);
  } else {
    data.SetStringKey(kWallpaperSolidColorKey, delegate_->GetSolidColor());
  }
  return data;
}

base::FilePath NTPCustomBackgroundImagesService::GetImageFilePath() {
  return delegate_->GetCustomBackgroundImageLocalFilePath();
}

void NTPCustomBackgroundImagesService::Shutdown() {
  delegate_.reset();
}

}  // namespace ntp_background_images

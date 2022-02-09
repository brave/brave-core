// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_IMAGES_NTP_CUSTOM_BACKGROUND_IMAGES_SERVICE_DELEGATE_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_IMAGES_NTP_CUSTOM_BACKGROUND_IMAGES_SERVICE_DELEGATE_H_

#include "brave/components/ntp_background_images/browser/ntp_custom_background_images_service.h"

class Profile;

namespace ntp_background_images {

class NTPCustomBackgroundImagesServiceDelegate
    : public NTPCustomBackgroundImagesService::Delegate {
 public:
  explicit NTPCustomBackgroundImagesServiceDelegate(Profile* profile);
  ~NTPCustomBackgroundImagesServiceDelegate() override;
  NTPCustomBackgroundImagesServiceDelegate(
      const NTPCustomBackgroundImagesServiceDelegate&) = delete;
  NTPCustomBackgroundImagesServiceDelegate& operator=(
      const NTPCustomBackgroundImagesServiceDelegate&) = delete;

 private:
  // NTPCustomBackgroundImagesService::Delegate overrides:
  bool IsCustomBackgroundEnabled() override;
  base::FilePath GetCustomBackgroundImageLocalFilePath() override;

  Profile* profile_ = nullptr;
};

}  // namespace ntp_background_images

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_IMAGES_NTP_CUSTOM_BACKGROUND_IMAGES_SERVICE_DELEGATE_H_

// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_NTP_CUSTOM_BACKGROUND_IMAGES_SERVICE_DELEGATE_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_NTP_CUSTOM_BACKGROUND_IMAGES_SERVICE_DELEGATE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/ntp_background_images/browser/ntp_custom_background_images_service.h"

class Profile;

class NTPCustomBackgroundImagesServiceDelegate
    : public ntp_background_images::NTPCustomBackgroundImagesService::Delegate {
 public:
  explicit NTPCustomBackgroundImagesServiceDelegate(Profile* profile);
  ~NTPCustomBackgroundImagesServiceDelegate() override;
  NTPCustomBackgroundImagesServiceDelegate(
      const NTPCustomBackgroundImagesServiceDelegate&) = delete;
  NTPCustomBackgroundImagesServiceDelegate& operator=(
      const NTPCustomBackgroundImagesServiceDelegate&) = delete;

 private:
  // NTPCustomBackgroundImagesService::Delegate overrides:
  bool IsCustomImageBackgroundEnabled() const override;
  base::FilePath GetCustomBackgroundImageLocalFilePath() const override;
  bool IsColorBackgroundEnabled() const override;
  std::string GetColor() const override;
  bool ShouldUseRandomValue() const override;
  bool HasPreferredBraveBackground() const override;
  base::Value::Dict GetPreferredBraveBackground() const override;

  raw_ptr<Profile> profile_ = nullptr;
};

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_NTP_CUSTOM_BACKGROUND_IMAGES_SERVICE_DELEGATE_H_

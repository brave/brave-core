// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_NTP_CUSTOM_BACKGROUND_IMAGES_SERVICE_DELEGATE_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_NTP_CUSTOM_BACKGROUND_IMAGES_SERVICE_DELEGATE_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/ntp_background_images/browser/ntp_custom_background_images_service.h"

class CustomBackgroundFileManager;
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
  FRIEND_TEST_ALL_PREFIXES(NTPCustomBackgroundImagesServiceDelegateUnitTest,
                           MigrationSuccess);
  FRIEND_TEST_ALL_PREFIXES(NTPCustomBackgroundImagesServiceDelegateUnitTest,
                           MigrationFail);

  bool ShouldMigrateCustomImagePref() const;
  void MigrateCustomImage(
      base::OnceCallback<void(bool)> callback = base::DoNothing());

  // NTPCustomBackgroundImagesService::Delegate overrides:
  bool IsCustomImageBackgroundEnabled() const override;
  base::FilePath GetCustomBackgroundImageLocalFilePath() const override;
  bool IsColorBackgroundEnabled() const override;
  std::string GetColor() const override;
  bool ShouldUseRandomValue() const override;
  bool HasPreferredBraveBackground() const override;
  base::Value::Dict GetPreferredBraveBackground() const override;

  raw_ptr<Profile> profile_ = nullptr;

  std::unique_ptr<CustomBackgroundFileManager> file_manager_;
};

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_NTP_CUSTOM_BACKGROUND_IMAGES_SERVICE_DELEGATE_H_

// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_BRAVE_NTP_CUSTOM_BACKGROUND_SERVICE_DELEGATE_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_BRAVE_NTP_CUSTOM_BACKGROUND_SERVICE_DELEGATE_H_

#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/ntp_background_images/browser/brave_ntp_custom_background_service.h"

class CustomBackgroundFileManager;
class Profile;

class BraveNTPCustomBackgroundServiceDelegate
    : public ntp_background_images::BraveNTPCustomBackgroundService::Delegate {
 public:
  explicit BraveNTPCustomBackgroundServiceDelegate(Profile* profile);
  ~BraveNTPCustomBackgroundServiceDelegate() override;
  BraveNTPCustomBackgroundServiceDelegate(
      const BraveNTPCustomBackgroundServiceDelegate&) = delete;
  BraveNTPCustomBackgroundServiceDelegate& operator=(
      const BraveNTPCustomBackgroundServiceDelegate&) = delete;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveNTPCustomBackgroundServiceDelegateUnitTest,
                           MigrationSuccess);
  FRIEND_TEST_ALL_PREFIXES(BraveNTPCustomBackgroundServiceDelegateUnitTest,
                           MigrationFail);

  bool ShouldMigrateCustomImagePref() const;
  void MigrateCustomImage(
      base::OnceCallback<void(bool)> callback = base::DoNothing());

  // BraveNTPCustomBackgroundService::Delegate overrides:
  bool IsCustomImageBackgroundEnabled() const override;
  base::FilePath GetCustomBackgroundImageLocalFilePath(
      const GURL& url) const override;
  GURL GetCustomBackgroundImageURL() const override;
  bool IsColorBackgroundEnabled() const override;
  std::string GetColor() const override;
  bool ShouldUseRandomValue() const override;
  bool HasPreferredBraveBackground() const override;
  base::Value::Dict GetPreferredBraveBackground() const override;

  raw_ptr<Profile> profile_ = nullptr;

  std::unique_ptr<CustomBackgroundFileManager> file_manager_;
};

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_BRAVE_NTP_CUSTOM_BACKGROUND_SERVICE_DELEGATE_H_

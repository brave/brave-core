// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ntp_background_images/ntp_custom_background_images_service_delegate.h"

#include "base/files/file_path.h"
#include "brave/browser/ntp_background_images/constants.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace ntp_background_images {

NTPCustomBackgroundImagesServiceDelegate::
    NTPCustomBackgroundImagesServiceDelegate(Profile* profile)
    : profile_(profile) {}

NTPCustomBackgroundImagesServiceDelegate::
    ~NTPCustomBackgroundImagesServiceDelegate() = default;

bool NTPCustomBackgroundImagesServiceDelegate::IsCustomBackgroundEnabled() {
  auto* prefs = profile_->GetPrefs();
  if (prefs->IsManagedPreference(prefs::kNtpCustomBackgroundDict))
    return false;

  return prefs->GetBoolean(kNewTabPageCustomBackgroundEnabled);
}

base::FilePath NTPCustomBackgroundImagesServiceDelegate::
    GetCustomBackgroundImageLocalFilePath() {
  if (!IsCustomBackgroundEnabled())
    return base::FilePath();
  return profile_->GetPath().AppendASCII(kSanitizedImageFileName);
}

}  // namespace ntp_background_images

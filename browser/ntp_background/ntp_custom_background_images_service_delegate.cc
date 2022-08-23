// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ntp_background/ntp_custom_background_images_service_delegate.h"

#include "base/files/file_path.h"
#include "brave/browser/ntp_background/constants.h"
#include "brave/browser/ntp_background/ntp_background_prefs.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

NTPCustomBackgroundImagesServiceDelegate::
    NTPCustomBackgroundImagesServiceDelegate(Profile* profile)
    : profile_(profile) {}

NTPCustomBackgroundImagesServiceDelegate::
    ~NTPCustomBackgroundImagesServiceDelegate() = default;

bool NTPCustomBackgroundImagesServiceDelegate::
    IsCustomImageBackgroundEnabled() {
  auto* prefs = profile_->GetPrefs();
  if (prefs->IsManagedPreference(prefs::kNtpCustomBackgroundDict))
    return false;

  return NTPBackgroundPrefs(prefs).IsCustomImageType();
}

base::FilePath NTPCustomBackgroundImagesServiceDelegate::
    GetCustomBackgroundImageLocalFilePath() {
  if (!IsCustomImageBackgroundEnabled())
    return base::FilePath();
  return profile_->GetPath().AppendASCII(
      ntp_background_images::kSanitizedImageFileName);
}

bool NTPCustomBackgroundImagesServiceDelegate::IsColorBackgroundEnabled() {
  return NTPBackgroundPrefs(profile_->GetPrefs()).IsColorType();
}

std::string NTPCustomBackgroundImagesServiceDelegate::GetColor() {
  if (!IsColorBackgroundEnabled())
    return {};

  auto selected_value =
      NTPBackgroundPrefs(profile_->GetPrefs()).GetSelectedValue();
  DCHECK(absl::holds_alternative<std::string>(selected_value));
  return absl::get<std::string>(selected_value);
}

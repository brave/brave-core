// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ntp_background_images/ntp_custom_background_images_service_delegate.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/background/ntp_background_data.h"
#include "chrome/browser/search/background/ntp_custom_background_service.h"
#include "chrome/browser/search/background/ntp_custom_background_service_factory.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace ntp_background_images {

NTPCustomBackgroundImagesServiceDelegate::
    NTPCustomBackgroundImagesServiceDelegate(Profile* profile)
    : profile_(profile) {}

NTPCustomBackgroundImagesServiceDelegate::
    ~NTPCustomBackgroundImagesServiceDelegate() = default;

bool NTPCustomBackgroundImagesServiceDelegate::IsCustomBackgroundEnabled() {
  return profile_->GetPrefs()->GetBoolean(
      prefs::kNtpCustomBackgroundLocalToDevice);
}

GURL NTPCustomBackgroundImagesServiceDelegate::GetCustomBackgroundImageURL() {
  DCHECK(IsCustomBackgroundEnabled());
  auto* service = NtpCustomBackgroundServiceFactory::GetForProfile(profile_);
  DCHECK(service);
  if (!service)
    return GURL();

  auto background = service->GetCustomBackground();
  if (!background)
    return GURL();

  return background->custom_background_url;
}

}  // namespace ntp_background_images

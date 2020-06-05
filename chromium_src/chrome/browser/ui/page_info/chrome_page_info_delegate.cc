/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/profile_util.h"

#include "../../../../../../chrome/browser/ui/page_info/chrome_page_info_delegate.cc"

bool ChromePageInfoDelegate::BraveShouldShowPermission(
    ContentSettingsType type) {
  if ((type == ContentSettingsType::PLUGINS ||
       type == ContentSettingsType::GEOLOCATION) &&
      brave::IsTorProfile(GetProfile())) {
    return false;
  }
  return true;
}

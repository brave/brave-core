/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/page_info/chrome_page_info_delegate.h"

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"

#include "src/chrome/browser/ui/page_info/chrome_page_info_delegate.cc"

bool ChromePageInfoDelegate::BraveShouldShowPermission(
    ContentSettingsType type) {
  if ((content_settings::IsShieldsContentSettingsType(type) ||
       type == ContentSettingsType::GEOLOCATION) &&
      GetProfile()->IsTor()) {
    return false;
  }
  return true;
}

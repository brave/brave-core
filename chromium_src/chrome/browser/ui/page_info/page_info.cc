/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/profile_util.h"
#include "content/public/browser/web_contents.h"
#include "chrome/browser/ui/page_info/page_info_ui.h"
#include "chrome/browser/profiles/profile.h"

namespace {

bool BraveShouldShowPermission(
    const PageInfoUI::PermissionInfo& info,
    content::WebContents* web_contents) {
  if ((info.type == ContentSettingsType::PLUGINS ||
       info.type == ContentSettingsType::GEOLOCATION) &&
      brave::IsTorProfile(
          Profile::FromBrowserContext(web_contents->GetBrowserContext()))) {
    return false;
  }

  return true;
}

}  // namespace

#include "../../../../../../chrome/browser/ui/page_info/page_info.cc"

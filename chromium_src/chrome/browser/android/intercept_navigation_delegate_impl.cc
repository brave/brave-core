/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace {

Profile* GetOriginalProfile() {
  return ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
}

bool ShouldPlayVideoInBrowser(const GURL& url) {
  if (!GetOriginalProfile()->GetPrefs()->GetBoolean(
          kPlayYTVideoInBrowserEnabled)) {
    return false;
  }

  if (url.host().find("youtube.com") != std::string::npos ||
      url.host().find("youtu.be") != std::string::npos) {
    return true;
  }

  return false;
}

}  // namespace

#define IGNORE_IF_PLAY_VIDEO_IN_BROWSER_IS_AVAILABLE(url) \
    if (ShouldPlayVideoInBrowser(url)) \
      return false;

#include "../../../../../chrome/browser/android/intercept_navigation_delegate_impl.cc"  // NOLINT

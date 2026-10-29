/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace {

// Brave offers the share omnibox icon only on web pages, and only while the
// share menu button is pinned.
bool ShouldOfferBraveShareIcon(content::WebContents& web_contents) {
  const GURL url = web_contents.GetLastCommittedURL();
  if (url.is_valid() && !url.SchemeIsHTTPOrHTTPS()) {
    return false;
  }
  return Profile::FromBrowserContext(web_contents.GetBrowserContext())
      ->GetPrefs()
      ->GetBoolean(prefs::kPinShareMenuButton);
}

}  // namespace

#include <chrome/browser/ui/sharing_hub/sharing_hub_bubble_controller_desktop_impl.cc>

/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view_location_bar_model_delegate.h"

#include "brave/browser/ui/views/split_view/split_view_location_bar.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/common/url_constants.h"

SplitViewLocationBarModelDelegate::SplitViewLocationBarModelDelegate() =
    default;

SplitViewLocationBarModelDelegate::~SplitViewLocationBarModelDelegate() =
    default;

// ChromeLocationBarModelDelegate:
content::WebContents* SplitViewLocationBarModelDelegate::GetActiveWebContents()
    const {
  return web_contents_;
}

bool SplitViewLocationBarModelDelegate::ShouldDisplayURL() const {
  content::NavigationEntry* entry = GetNavigationEntry();
  if (entry && !entry->IsInitialEntry()) {
    // We don't want to hide chrome://newtab url for this location bar.
    const auto is_ntp = [](const GURL& url) {
      return url.SchemeIs(content::kChromeUIScheme) &&
             url.host() == chrome::kChromeUINewTabHost;
    };
    if (is_ntp(entry->GetVirtualURL()) || is_ntp(entry->GetURL())) {
      return true;
    }
  }

  return ChromeLocationBarModelDelegate::ShouldDisplayURL();
}

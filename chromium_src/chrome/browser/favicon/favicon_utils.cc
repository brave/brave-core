// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/favicon/favicon_utils.h"

#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/common/url_constants.h"

// Allow brave internal pages to break out of favicon themeing
#define ShouldThemifyFaviconForEntry ShouldThemifyFaviconForEntry_ChromiumImpl
#include "src/chrome/browser/favicon/favicon_utils.cc"
#undef ShouldThemifyFaviconForEntry

namespace favicon {

bool ShouldThemifyFaviconForEntry(content::NavigationEntry* entry) {
  const GURL& virtual_url = entry->GetVirtualURL();
  // Don't theme for certain brave favicons which are full color
  if (virtual_url.SchemeIs(content::kChromeUIScheme) &&
      (virtual_url.host_piece() == kRewardsPageHost ||
       virtual_url.host_piece() == kWalletPageHost ||
       virtual_url.host_piece() == kAIChatUIHost)) {
    return false;
  }
  return ShouldThemifyFaviconForEntry_ChromiumImpl(entry);
}

}  // namespace favicon
